// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ReadReceiptsModel.h"

#include <QLocale>

#include "Cache.h"
#include "Logging.h"
#include "Utils.h"

ReadReceiptsModel::ReadReceiptsModel(QString event_id, QString room_id, QObject *parent)
  : QAbstractListModel{parent}
  , event_id_{event_id}
  , room_id_{room_id}
{
        try {
                addUsers(cache::readReceipts(event_id, room_id));
        } catch (const lmdb::error &) {
                nhlog::db()->warn("failed to retrieve read receipts for {} {}",
                                  event_id.toStdString(),
                                  room_id_.toStdString());

                return;
        }
}

ReadReceiptsModel::~ReadReceiptsModel()
{
        for (const auto &item : readReceipts_)
                item->deleteLater();
}

QHash<int, QByteArray>
ReadReceiptsModel::roleNames() const
{
        return {{Mxid, "mxid"},
                {DisplayName, "displayName"},
                {AvatarUrl, "avatarUrl"},
                {Timestamp, "timestamp"}};
}

QVariant
ReadReceiptsModel::data(const QModelIndex &index, int role) const
{
        if (!index.isValid() || index.row() >= (int)readReceipts_.size() || index.row() < 0)
                return {};

        switch (role) {
        case Mxid:
                return readReceipts_[index.row()]->mxid();
        case DisplayName:
                return readReceipts_[index.row()]->displayName();
        case AvatarUrl:
                return readReceipts_[index.row()]->avatarUrl();
        case Timestamp:
                // the uint64_t to QVariant conversion was ambiguous, so...
                return readReceipts_[index.row()]->timestamp();
        default:
                return {};
        }
}

void
ReadReceiptsModel::addUsers(
  const std::multimap<uint64_t, std::string, std::greater<uint64_t>> &users)
{
        std::multimap<uint64_t, std::string, std::greater<uint64_t>> unshown;
        for (const auto &user : users) {
                if (users_.find(user.first) == users_.end())
                        unshown.emplace(user);
        }

        beginInsertRows(
          QModelIndex{}, readReceipts_.length(), readReceipts_.length() + unshown.size() - 1);

        for (const auto &user : unshown)
                readReceipts_.push_back(
                  new ReadReceipt{QString::fromStdString(user.second), room_id_, user.first, this});

        users_.merge(unshown);

        endInsertRows();
}

ReadReceipt::ReadReceipt(QString mxid, QString room_id, uint64_t timestamp, QObject *parent)
  : QObject{parent}
  , mxid_{mxid}
  , room_id_{room_id}
  , displayName_{cache::displayName(room_id_, mxid_)}
  , avatarUrl_{cache::avatarUrl(room_id_, mxid_)}
  , timestamp_{timestamp}
{}

QString
ReadReceipt::timestamp() const
{
        return dateFormat(QDateTime::fromMSecsSinceEpoch(timestamp_));
}

QString
ReadReceipt::dateFormat(const QDateTime &then) const
{
        auto now  = QDateTime::currentDateTime();
        auto days = then.daysTo(now);

        if (days == 0)
                return tr("Today %1")
                  .arg(QLocale::system().toString(then.time(), QLocale::ShortFormat));
        else if (days < 2)
                return tr("Yesterday %1")
                  .arg(QLocale::system().toString(then.time(), QLocale::ShortFormat));
        else if (days < 7)
                return QString("%1 %2")
                  .arg(then.toString("dddd"))
                  .arg(QLocale::system().toString(then.time(), QLocale::ShortFormat));

        return QLocale::system().toString(then.time(), QLocale::ShortFormat);
}
