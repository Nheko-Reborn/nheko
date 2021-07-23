// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef READRECEIPTSMODEL_H
#define READRECEIPTSMODEL_H

#include <QAbstractListModel>
#include <QObject>
#include <QString>

class ReadReceipt : public QObject
{
        Q_OBJECT

        Q_PROPERTY(QString mxid READ mxid CONSTANT)
        Q_PROPERTY(QString displayName READ displayName NOTIFY displayNameChanged)
        Q_PROPERTY(QString avatarUrl READ avatarUrl NOTIFY avatarUrlChanged)
        Q_PROPERTY(QString timestamp READ timestamp CONSTANT)

public:
        explicit ReadReceipt(QString mxid,
                             QString room_id,
                             uint64_t timestamp,
                             QObject *parent = nullptr);

        QString mxid() const { return mxid_; }
        QString displayName() const { return displayName_; }
        QString avatarUrl() const { return avatarUrl_; }
        QString timestamp() const;

signals:
        void displayNameChanged();
        void avatarUrlChanged();

private:
        QString dateFormat(const QDateTime &then) const;

        QString mxid_;
        QString room_id_;
        QString displayName_;
        QString avatarUrl_;
        uint64_t timestamp_;
};

class ReadReceiptsModel : public QAbstractListModel
{
        Q_OBJECT

        Q_PROPERTY(QString eventId READ eventId CONSTANT)
        Q_PROPERTY(QString roomId READ roomId CONSTANT)

public:
        enum Roles
        {
                Mxid,
                DisplayName,
                AvatarUrl,
                Timestamp,
        };

        explicit ReadReceiptsModel(QString event_id, QString room_id, QObject *parent = nullptr);
        ~ReadReceiptsModel() override;

        QString eventId() const { return event_id_; }
        QString roomId() const { return room_id_; }

        QHash<int, QByteArray> roleNames() const override;
        int rowCount(const QModelIndex &parent) const override
        {
                Q_UNUSED(parent)
                return readReceipts_.size();
        }
        QVariant data(const QModelIndex &index, int role) const override;

public slots:
        void addUsers(const std::multimap<uint64_t, std::string, std::greater<uint64_t>> &users);

private:
        QString event_id_;
        QString room_id_;
        QVector<ReadReceipt *> readReceipts_;
        std::multimap<uint64_t, std::string, std::greater<uint64_t>> users_;
};

#endif // READRECEIPTSMODEL_H
