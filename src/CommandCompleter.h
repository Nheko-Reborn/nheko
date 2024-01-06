// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QString>

class CommandCompleter final : public QAbstractListModel
{
public:
    enum Roles
    {
        Name = Qt::UserRole,
        Description,
    };

    enum Commands
    {
        Me,
        React,
        Join,
        Knock,
        Part,
        Leave,
        Invite,
        Kick,
        Ban,
        Unban,
        Redact,
        Roomnick,
        Shrug,
        Fliptable,
        Unfliptable,
        Sovietflip,
        ClearTimeline,
        ResetState,
        RotateMegolmSession,
        Md,
        Cmark,
        Plain,
        Rainbow,
        RainbowMe,
        Notice,
        RainbowNotice,
        Confetti,
        RainbowConfetti,
        Rainfall,
        Msgtype,
        Goto,
        ConvertToDm,
        ConvertToRoom,
        Ignore,
        Unignore,
        COUNT,
    };

    CommandCompleter(QObject *parent = nullptr);
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        (void)parent;
        return (int)Commands::COUNT;
    }
    QVariant data(const QModelIndex &index, int role) const override;
};
