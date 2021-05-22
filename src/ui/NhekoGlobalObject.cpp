// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "NhekoGlobalObject.h"

#include <QDesktopServices>
#include <QUrl>

#include "Cache_p.h"
#include "ChatPage.h"
#include "Logging.h"
#include "UserSettingsPage.h"
#include "Utils.h"

Nheko::Nheko()
{
        connect(
          UserSettings::instance().get(), &UserSettings::themeChanged, this, &Nheko::colorsChanged);
        connect(ChatPage::instance(), &ChatPage::contentLoaded, this, &Nheko::updateUserProfile);
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
        if (url.scheme() == "https" && url.host() == "matrix.to") {
                // handle matrix.to links internally
                QString p = url.fragment(QUrl::FullyEncoded);
                if (p.startsWith("/"))
                        p.remove(0, 1);

                auto temp = p.split("?");
                QString query;
                if (temp.size() >= 2)
                        query = QUrl::fromPercentEncoding(temp.takeAt(1).toUtf8());

                temp            = temp.first().split("/");
                auto identifier = QUrl::fromPercentEncoding(temp.takeFirst().toUtf8());
                QString eventId = QUrl::fromPercentEncoding(temp.join('/').toUtf8());
                if (!identifier.isEmpty()) {
                        if (identifier.startsWith("@")) {
                                QByteArray uri =
                                  "matrix:u/" + QUrl::toPercentEncoding(identifier.remove(0, 1));
                                if (!query.isEmpty())
                                        uri.append("?" + query.toUtf8());
                                ChatPage::instance()->handleMatrixUri(QUrl::fromEncoded(uri));
                        } else if (identifier.startsWith("#")) {
                                QByteArray uri =
                                  "matrix:r/" + QUrl::toPercentEncoding(identifier.remove(0, 1));
                                if (!eventId.isEmpty())
                                        uri.append("/e/" +
                                                   QUrl::toPercentEncoding(eventId.remove(0, 1)));
                                if (!query.isEmpty())
                                        uri.append("?" + query.toUtf8());
                                ChatPage::instance()->handleMatrixUri(QUrl::fromEncoded(uri));
                        } else if (identifier.startsWith("!")) {
                                QByteArray uri = "matrix:roomid/" +
                                                 QUrl::toPercentEncoding(identifier.remove(0, 1));
                                if (!eventId.isEmpty())
                                        uri.append("/e/" +
                                                   QUrl::toPercentEncoding(eventId.remove(0, 1)));
                                if (!query.isEmpty())
                                        uri.append("?" + query.toUtf8());
                                ChatPage::instance()->handleMatrixUri(QUrl::fromEncoded(uri));
                        }
                }
        } else {
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
