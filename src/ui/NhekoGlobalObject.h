// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QFontDatabase>
#include <QObject>
#include <QPalette>
#include <QWindow>

#include "AliasEditModel.h"
#include "PowerlevelsEditModels.h"
#include "RoomSummary.h"
#include "Theme.h"
#include "UserProfile.h"

class Nheko : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QPalette colors READ colors NOTIFY colorsChanged)
    Q_PROPERTY(QPalette inactiveColors READ inactiveColors NOTIFY colorsChanged)
    Q_PROPERTY(Theme theme READ theme NOTIFY colorsChanged)
    Q_PROPERTY(int avatarSize READ avatarSize CONSTANT)
    Q_PROPERTY(int paddingSmall READ paddingSmall CONSTANT)
    Q_PROPERTY(int paddingMedium READ paddingMedium CONSTANT)
    Q_PROPERTY(int paddingLarge READ paddingLarge CONSTANT)
    Q_PROPERTY(int tooltipDelay READ tooltipDelay CONSTANT)

    Q_PROPERTY(UserProfile *currentUser READ currentUser NOTIFY profileChanged)

public:
    Nheko();

    QPalette colors() const;
    QPalette inactiveColors() const;
    Theme theme() const;

    int avatarSize() const { return 40; }

    int paddingSmall() const { return 4; }
    int paddingMedium() const { return 8; }
    int paddingLarge() const { return 20; }

    int tooltipDelay() const;

    UserProfile *currentUser() const;

    Q_INVOKABLE QFont monospaceFont() const
    {
        return QFontDatabase::systemFont(QFontDatabase::FixedFont);
    }
    Q_INVOKABLE void openLink(QString link) const;
    Q_INVOKABLE void setStatusMessage(QString msg) const;
    Q_INVOKABLE void showUserSettingsPage() const;
    Q_INVOKABLE void logout() const;
    Q_INVOKABLE void createRoom(bool space,
                                QString name,
                                QString topic,
                                QString aliasLocalpart,
                                bool isEncrypted,
                                int preset);
    Q_INVOKABLE PowerlevelEditingModels *editPowerlevels(QString room_id_) const
    {
        return new PowerlevelEditingModels(room_id_);
    }
    Q_INVOKABLE AliasEditingModel *editAliases(QString room_id_) const
    {
        return new AliasEditingModel(room_id_.toStdString());
    }
    Q_INVOKABLE void setTransientParent(QWindow *window, QWindow *parentWindow) const;

public slots:
    void updateUserProfile();

signals:
    void colorsChanged();
    void profileChanged();

    void openLogoutDialog();
    void openJoinRoomDialog();
    void joinRoom(QString roomId, QString reason = "");

    void showRoomJoinPrompt(RoomSummary *summary);

private:
    QScopedPointer<UserProfile> currentUser_;
};
