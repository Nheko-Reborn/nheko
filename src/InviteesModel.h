// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef INVITEESMODEL_H
#define INVITEESMODEL_H

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QVector>

#include "timeline/TimelineModel.h"

class Invitee final : public QObject
{
    Q_OBJECT

public:
    Invitee(QString mxid,
            QString displayName = "",
            QString avatarUrl   = "",
            QObject *parent     = nullptr);

signals:
    void userInfoLoaded();

private:
    const QString mxid_;
    QString displayName_;
    QString avatarUrl_;

    friend class InviteesModel;
};

class InviteesModel final : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(TimelineModel *room READ room CONSTANT)

public:
    enum Roles
    {
        Mxid,
        DisplayName,
        AvatarUrl,
    };

    InviteesModel(TimelineModel *room, QObject *parent = nullptr);

    TimelineModel *room() const { return room_; }

    Q_INVOKABLE void addUser(QString mxid, QString displayName = "", QString avatarUrl = "");
    Q_INVOKABLE void removeUser(QString mxid);

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] int rowCount(const QModelIndex & = QModelIndex()) const override
    {
        return (int)invitees_.size();
    }
    [[nodiscard]] QVariant
    data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QStringList mxids();

signals:
    void accept();
    void countChanged();

private:
    QVector<Invitee *> invitees_;
    TimelineModel *room_;
};

#endif // INVITEESMODEL_H
