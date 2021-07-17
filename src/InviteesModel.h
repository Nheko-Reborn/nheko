// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef INVITEESMODEL_H
#define INVITEESMODEL_H

#include <QAbstractListModel>
#include <QVector>

class Invitee : public QObject
{
        Q_OBJECT

public:
        Invitee(const QString &mxid, QObject *parent = nullptr);

signals:
        void userInfoLoaded();

private:
        const QString mxid_;
        QString displayName_;
        QString avatarUrl_;

        friend class InviteesModel;
};

class InviteesModel : public QAbstractListModel
{
        Q_OBJECT

public:
        enum Roles
        {
                Mxid,
                DisplayName,
                AvatarUrl,
        };

        InviteesModel(QObject *parent = nullptr);

        Q_INVOKABLE void addUser(QString mxid);

        QHash<int, QByteArray> roleNames() const override;
        int rowCount(const QModelIndex & = QModelIndex()) const override
        {
                return (int)invitees_.size();
        }
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        QStringList mxids();

signals:
        void accept();

private:
        QVector<Invitee *> invitees_;
};

#endif // INVITEESMODEL_H
