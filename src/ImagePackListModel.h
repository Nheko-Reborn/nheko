// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QSharedPointer>

#include "SingleImagePackModel.h"

class ImagePackListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(bool containsAccountPack READ containsAccountPack CONSTANT)
public:
    enum Roles
    {
        DisplayName = Qt::UserRole,
        AvatarUrl,
        FromAccountData,
        FromCurrentRoom,
        FromSpace,
        StateKey,
        RoomId,
    };

    ImagePackListModel(const std::string &roomId, QObject *parent = nullptr);
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    Q_INVOKABLE SingleImagePackModel *packAt(int row);
    Q_INVOKABLE SingleImagePackModel *newPack(bool inRoom);

    bool containsAccountPack() const;

private:
    std::string room_id;

    std::vector<QSharedPointer<SingleImagePackModel>> packs;
};
