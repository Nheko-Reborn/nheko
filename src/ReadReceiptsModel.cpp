// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ReadReceiptsModel.h"

#include <QLocale>

#include "Cache.h"
#include "Cache_p.h"
#include "Logging.h"
#include "Utils.h"

ReadReceiptsModel::ReadReceiptsModel(QString event_id, QString room_id, QObject *parent)
  : QAbstractListModel{parent}
  , event_id_{event_id}
  , room_id_{room_id}
{
        try {
                addUsers(cache::readReceipts(event_id_, room_id_));
        } catch (const lmdb::error &) {
                nhlog::db()->warn("failed to retrieve read receipts for {} {}",
                                  event_id_.toStdString(),
                                  room_id_.toStdString());

                return;
        }

        connect(cache::client(), &Cache::newReadReceipts, this, &ReadReceiptsModel::update);
}

void
ReadReceiptsModel::update()
{
        try {
                addUsers(cache::readReceipts(event_id_, room_id_));
        } catch (const lmdb::error &) {
                nhlog::db()->warn("failed to retrieve read receipts for {} {}",
                                  event_id_.toStdString(),
                                  room_id_.toStdString());

                return;
        }
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
                return readReceipts_[index.row()].first;
        case DisplayName:
                return cache::displayName(room_id_, readReceipts_[index.row()].first);
        case AvatarUrl:
                return cache::avatarUrl(room_id_, readReceipts_[index.row()].first);
        case Timestamp:
                return dateFormat(readReceipts_[index.row()].second);
        default:
                return {};
        }
}

void
ReadReceiptsModel::addUsers(
  const std::multimap<uint64_t, std::string, std::greater<uint64_t>> &users)
{
        auto oldLen = readReceipts_.length();

        beginInsertRows(QModelIndex{}, oldLen, users.size() - 1);

        readReceipts_.clear();
        for (const auto &user : users) {
                readReceipts_.push_back({QString::fromStdString(user.second),
                                         QDateTime::fromMSecsSinceEpoch(user.first)});
        }

        std::sort(readReceipts_.begin(),
                  readReceipts_.end(),
                  [](const QPair<QString, QDateTime> &a, const QPair<QString, QDateTime> &b) {
                          return a.second > b.second;
                  });

        endInsertRows();

        emit dataChanged(index(0), index(oldLen - 1));
}

QString
ReadReceiptsModel::dateFormat(const QDateTime &then) const
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
