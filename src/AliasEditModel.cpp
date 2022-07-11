// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AliasEditModel.h"

#include <QSharedPointer>

#include <set>

#include <mtx/responses/common.hpp>

#include "Cache.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "timeline/Permissions.h"
#include "timeline/TimelineModel.h"

AliasEditingModel::AliasEditingModel(const std::string &rid, QObject *parent)
  : QAbstractListModel(parent)
  , room_id(rid)
  , aliasEvent(cache::client()
                 ->getStateEvent<mtx::events::state::CanonicalAlias>(room_id)
                 .value_or(mtx::events::StateEvent<mtx::events::state::CanonicalAlias>{})
                 .content)
  , canSendStateEvent(
      Permissions(QString::fromStdString(rid)).canChange(qml_mtx_events::CanonicalAlias))
{
    std::set<std::string> seen_aliases;

    if (!aliasEvent.alias.empty()) {
        aliases.push_back(Entry{aliasEvent.alias, true, true, false});
        seen_aliases.insert(aliasEvent.alias);
    }

    for (const auto &alias : aliasEvent.alt_aliases) {
        if (!seen_aliases.count(alias)) {
            aliases.push_back(Entry{alias, false, true, false});
            seen_aliases.insert(alias);
        }
    }

    for (const auto &alias : qAsConst(aliases)) {
        fetchAliasesStatus(alias.alias);
    }
    fetchPublishedAliases();
}

void
AliasEditingModel::fetchPublishedAliases()
{
    auto job = QSharedPointer<FetchPublishedAliasesJob>::create();
    connect(job.data(),
            &FetchPublishedAliasesJob::advertizedAliasesFetched,
            this,
            &AliasEditingModel::updatePublishedAliases);
    http::client()->list_room_aliases(
      room_id, [job](const mtx::responses::Aliases &aliasesFetched, mtx::http::RequestErr) {
          emit job->advertizedAliasesFetched(std::move(aliasesFetched.aliases));
      });
}

void
AliasEditingModel::fetchAliasesStatus(const std::string &alias)
{
    auto job = QSharedPointer<FetchPublishedAliasesJob>::create();
    connect(
      job.data(), &FetchPublishedAliasesJob::aliasFetched, this, &AliasEditingModel::updateAlias);
    http::client()->resolve_room_alias(
      alias, [job, alias](const mtx::responses::RoomId &roomIdFetched, mtx::http::RequestErr e) {
          if (!e)
              emit job->aliasFetched(alias, std::move(roomIdFetched.room_id));
      });
}

QHash<int, QByteArray>
AliasEditingModel::roleNames() const
{
    return {
      {Name, "name"},
      {IsPublished, "isPublished"},
      {IsCanonical, "isCanonical"},
      {IsAdvertized, "isAdvertized"},
    };
}

QVariant
AliasEditingModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= aliases.size())
        return {};

    const auto &entry = aliases.at(index.row());

    switch (role) {
    case Name:
        return QString::fromStdString(entry.alias);
    case IsPublished:
        return entry.published;
    case IsCanonical:
        return entry.canonical;
    case IsAdvertized:
        return entry.advertized;
    }

    return {};
}

bool
AliasEditingModel::deleteAlias(int row)
{
    if (row < 0 || row >= aliases.size() || aliases.at(row).alias.empty())
        return false;

    auto alias = aliases.at(row);

    beginRemoveRows(QModelIndex(), row, row);
    aliases.remove(row);
    endRemoveRows();

    if (alias.published)
        http::client()->delete_room_alias(alias.alias, [alias](mtx::http::RequestErr e) {
            if (e) {
                nhlog::net()->error("Failed to delete {}: {}", alias.alias, *e);
                ChatPage::instance()->showNotification(
                  tr("Failed to unpublish alias %1: %2")
                    .arg(QString::fromStdString(alias.alias),
                         QString::fromStdString(e->matrix_error.error)));
            }
        });

    if (aliasEvent.alias == alias.alias)
        aliasEvent.alias.clear();

    for (size_t i = 0; i < aliasEvent.alt_aliases.size(); i++) {
        if (aliasEvent.alt_aliases[i] == alias.alias) {
            aliasEvent.alt_aliases.erase(aliasEvent.alt_aliases.begin() + i);
            break;
        }
    }

    return true;
}

void
AliasEditingModel::addAlias(QString newAlias)
{
    const auto aliasStr = newAlias.toStdString();
    for (const auto &e : qAsConst(aliases)) {
        if (e.alias == aliasStr) {
            return;
        }
    }

    beginInsertRows(QModelIndex(), aliases.length(), aliases.length());
    if (aliasEvent.alias.empty())
        aliasEvent.alias = aliasStr;
    else
        aliasEvent.alt_aliases.push_back(aliasStr);
    aliases.push_back(
      Entry{aliasStr, aliasEvent.alias.empty() && canSendStateEvent, canSendStateEvent, false});
    endInsertRows();

    auto job = QSharedPointer<FetchPublishedAliasesJob>::create();
    connect(
      job.data(), &FetchPublishedAliasesJob::aliasFetched, this, &AliasEditingModel::updateAlias);
    auto room = room_id;
    http::client()->add_room_alias(
      aliasStr, room_id, [job, aliasStr, room](mtx::http::RequestErr e) {
          if (e) {
              nhlog::net()->error("Failed to publish {}: {}", aliasStr, *e);
              ChatPage::instance()->showNotification(
                tr("Failed to unpublish alias %1: %2")
                  .arg(QString::fromStdString(aliasStr),
                       QString::fromStdString(e->matrix_error.error)));
              emit job->aliasFetched(aliasStr, "");
          } else {
              emit job->aliasFetched(aliasStr, room);
          }
      });
}

void
AliasEditingModel::makeCanonical(int row)
{
    if (!canSendStateEvent || row < 0 || row >= aliases.size() || aliases.at(row).alias.empty())
        return;

    auto moveAlias = aliases.at(row).alias;

    if (!aliasEvent.alias.empty()) {
        for (qsizetype i = 0; i < aliases.size(); i++) {
            if (moveAlias == aliases[i].alias) {
                if (aliases[i].canonical) {
                    aliases[i].canonical = false;
                    aliasEvent.alt_aliases.push_back(aliasEvent.alias);
                    emit dataChanged(index(i), index(i), {IsCanonical});
                }
                break;
            }
        }
    }

    aliasEvent.alias = moveAlias;
    for (auto i = aliasEvent.alt_aliases.begin(); i != aliasEvent.alt_aliases.end(); ++i) {
        if (*i == moveAlias) {
            aliasEvent.alt_aliases.erase(i);
            break;
        }
    }
    aliases[row].canonical  = true;
    aliases[row].advertized = true;
    emit dataChanged(index(row), index(row), {IsCanonical, IsAdvertized});
}

void
AliasEditingModel::togglePublish(int row)
{
    if (row < 0 || row >= aliases.size() || aliases.at(row).alias.empty())
        return;
    auto aliasStr = aliases[row].alias;

    auto job = QSharedPointer<FetchPublishedAliasesJob>::create();
    connect(
      job.data(), &FetchPublishedAliasesJob::aliasFetched, this, &AliasEditingModel::updateAlias);
    auto room = room_id;
    if (!aliases[row].published)
        http::client()->add_room_alias(
          aliasStr, room_id, [job, aliasStr, room](mtx::http::RequestErr e) {
              if (e) {
                  nhlog::net()->error("Failed to publish {}: {}", aliasStr, *e);
                  ChatPage::instance()->showNotification(
                    tr("Failed to unpublish alias %1: %2")
                      .arg(QString::fromStdString(aliasStr),
                           QString::fromStdString(e->matrix_error.error)));
                  emit job->aliasFetched(aliasStr, "");
              } else {
                  emit job->aliasFetched(aliasStr, room);
              }
          });
    else
        http::client()->delete_room_alias(aliasStr, [job, aliasStr, room](mtx::http::RequestErr e) {
            if (e) {
                nhlog::net()->error("Failed to unpublish {}: {}", aliasStr, *e);
                ChatPage::instance()->showNotification(
                  tr("Failed to unpublish alias %1: %2")
                    .arg(QString::fromStdString(aliasStr),
                         QString::fromStdString(e->matrix_error.error)));
                emit job->aliasFetched(aliasStr, room);
            } else {
                emit job->aliasFetched(aliasStr, "");
            }
        });
}

void
AliasEditingModel::toggleAdvertize(int row)
{
    if (!canSendStateEvent || row < 0 || row >= aliases.size() || aliases.at(row).alias.empty())
        return;

    auto &moveAlias = aliases[row];
    if (aliasEvent.alias == moveAlias.alias) {
        moveAlias.canonical  = false;
        moveAlias.advertized = false;
        aliasEvent.alias.clear();
        emit dataChanged(index(row), index(row), {IsAdvertized, IsCanonical});
    } else if (moveAlias.advertized) {
        for (auto i = aliasEvent.alt_aliases.begin(); i != aliasEvent.alt_aliases.end(); ++i) {
            if (*i == moveAlias.alias) {
                aliasEvent.alt_aliases.erase(i);
                moveAlias.advertized = false;
                emit dataChanged(index(row), index(row), {IsAdvertized});
                break;
            }
        }
    } else {
        aliasEvent.alt_aliases.push_back(moveAlias.alias);
        moveAlias.advertized = true;
        emit dataChanged(index(row), index(row), {IsAdvertized});
    }
}

void
AliasEditingModel::updateAlias(std::string alias, std::string target)
{
    for (qsizetype i = 0; i < aliases.size(); i++) {
        auto &e = aliases[i];
        if (e.alias == alias) {
            e.published = (target == room_id);
            emit dataChanged(index(i), index(i), {IsPublished});
        }
    }
}

void
AliasEditingModel::updatePublishedAliases(std::vector<std::string> advAliases)
{
    for (const auto &advAlias : advAliases) {
        bool found = false;
        for (qsizetype i = 0; i < aliases.size(); i++) {
            auto &alias = aliases[i];
            if (alias.alias == advAlias) {
                alias.published = true;
                emit dataChanged(index(i), index(i), {IsPublished});
                found = true;
                break;
            }
        }

        if (!found) {
            beginInsertRows(QModelIndex(), aliases.size(), aliases.size());
            aliases.push_back(Entry{advAlias, false, false, true});
            endInsertRows();
        }
    }
}

void
AliasEditingModel::commit()
{
    if (!canSendStateEvent)
        return;

    http::client()->send_state_event(
      room_id, aliasEvent, [](const mtx::responses::EventId &, mtx::http::RequestErr e) {
          if (e) {
              nhlog::net()->error("Failed to send Alias event: {}", *e);
              ChatPage::instance()->showNotification(
                tr("Failed to update aliases: %1")
                  .arg(QString::fromStdString(e->matrix_error.error)));
          }
      });
}
