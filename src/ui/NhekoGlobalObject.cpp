// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "NhekoGlobalObject.h"

#include <QApplication>
#include <QDesktopServices>
#include <QStyle>
#include <QUrl>

#include "Cache_p.h"
#include "ChatPage.h"
#include "Logging.h"
#include "MainWindow.h"
#include "UserSettingsPage.h"
#include "Utils.h"
#include "voip/WebRTCSession.h"

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
        currentUser_.reset(new UserProfile(
          QLatin1String(""), utils::localUser(), ChatPage::instance()->timelineManager()));
    else
        currentUser_.reset();
    emit profileChanged();
}

QPalette
Nheko::colors() const
{
    return Theme::paletteFromTheme(UserSettings::instance()->theme());
}

QPalette
Nheko::inactiveColors() const
{
    auto theme = UserSettings::instance()->theme();
    if (theme == QLatin1String("light")) {
        static QPalette lightInactive = [] {
            auto lightInactive = Theme::paletteFromTheme(u"light");
            lightInactive.setCurrentColorGroup(QPalette::ColorGroup::Inactive);
            return lightInactive;
        }();
        return lightInactive;
    } else if (theme == QLatin1String("dark")) {
        static QPalette darkInactive = [] {
            auto darkInactive = Theme::paletteFromTheme(u"dark");
            darkInactive.setCurrentColorGroup(QPalette::ColorGroup::Inactive);
            return darkInactive;
        }();
        return darkInactive;
    } else {
        static QPalette originalInactive = [] {
            auto originalInactive = Theme::paletteFromTheme(u"system");
            originalInactive.setCurrentColorGroup(QPalette::ColorGroup::Inactive);
            return originalInactive;
        }();
        return originalInactive;
    }
}

Theme
Nheko::theme() const
{
    return Theme(UserSettings::instance()->theme());
}

int
Nheko::tooltipDelay() const
{
    return QApplication::style()->styleHint(QStyle::StyleHint::SH_ToolTip_WakeUpDelay);
}

void
Nheko::openLink(QString link) const
{
    QUrl url(link);
    // Open externally if we couldn't handle it internally
    if (!ChatPage::instance()->handleMatrixUri(url)) {
        static const QStringList allowedUrlSchemes = {
          QStringLiteral("http"),
          QStringLiteral("https"),
          QStringLiteral("mailto"),
        };

        if (allowedUrlSchemes.contains(url.scheme()))
            QDesktopServices::openUrl(url);
        else
            nhlog::ui()->warn("Url '{}' not opened, because the scheme is not in the allow list",
                              url.toDisplayString().toStdString());
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
