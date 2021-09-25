// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "NhekoGlobalObject.h"

#include <QDesktopServices>
#include <QUrl>
#include <QWindow>

#include "Cache_p.h"
#include "ChatPage.h"
#include "Logging.h"
#include "MainWindow.h"
#include "UserSettingsPage.h"
#include "Utils.h"
#include "WebRTCSession.h"

Nheko::Nheko()
{
    connect(
      UserSettings::instance().get(), &UserSettings::themeChanged, this, &Nheko::colorsChanged);
    connect(ChatPage::instance(), &ChatPage::contentLoaded, this, &Nheko::updateUserProfile);
    connect(this, &Nheko::joinRoom, ChatPage::instance(), &ChatPage::joinRoom);
}

void
Nheko::updateUserProfile()
{
    if (cache::client() && cache::client()->isInitialized())
        currentUser_.reset(
          new UserProfile("", utils::localUser(), ChatPage::instance()->timelineManager()));
    else
        currentUser_.reset();
    emit profileChanged();
}

QPalette
Nheko::colors() const
{
    return Theme::paletteFromTheme(UserSettings::instance()->theme().toStdString());
}

QPalette
Nheko::inactiveColors() const
{
    auto p = colors();
    p.setCurrentColorGroup(QPalette::ColorGroup::Inactive);
    return p;
}

Theme
Nheko::theme() const
{
    return Theme(UserSettings::instance()->theme().toStdString());
}

void
Nheko::openLink(QString link) const
{
    QUrl url(link);
    // Open externally if we couldn't handle it internally
    if (!ChatPage::instance()->handleMatrixUri(url)) {
        QDesktopServices::openUrl(url);
    }
}
void
Nheko::setStatusMessage(QString msg) const
{
    ChatPage::instance()->setStatus(msg);
}

UserProfile *
Nheko::currentUser() const
{
    nhlog::ui()->debug("Profile requested");

    return currentUser_.get();
}

void
Nheko::showUserSettingsPage() const
{
    ChatPage::instance()->showUserSettingsPage();
}

void
Nheko::logout() const
{
    ChatPage::instance()->initiateLogout();
}

void
Nheko::openCreateRoomDialog() const
{
    MainWindow::instance()->openCreateRoomDialog(
      [](const mtx::requests::CreateRoom &req) { ChatPage::instance()->createRoom(req); });
}

void
Nheko::reparent(QWindow *win) const
{
    win->setTransientParent(MainWindow::instance()->windowHandle());
}
