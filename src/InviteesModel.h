// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
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
    Invitee(QString mxid, QObject *parent = nullptr);

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

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles
    {
        Mxid,
        DisplayName,
        AvatarUrl,
    };

    InviteesModel(QObject *parent = nullptr);

    Q_INVOKABLE void addUser(QString mxid);
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
};

#endif // INVITEESMODEL_H
