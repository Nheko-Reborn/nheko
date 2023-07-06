// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>

#include <QHash>
#include <QQuickView>
#include <QSharedPointer>
#include <QSystemTrayIcon>

#include "UserSettingsPage.h"
#include "dock/Dock.h"

#include "jdenticoninterface.h"

class ChatPage;
class RegisterPage;
class WelcomePage;

class TrayIcon;
class UserSettings;
class MxcImageProvider;

namespace mtx {
namespace requests {
struct CreateRoom;
}
}

namespace dialogs {
class CreateRoom;
class InviteUsers;
class MemberList;
}

class NhekoFixupPaletteEventFilter final : public QObject
{
    Q_OBJECT

public:
    NhekoFixupPaletteEventFilter(QObject *parent)
      : QObject(parent)
    {
    }

    bool eventFilter(QObject *obj, QEvent *event) override;
};

class MainWindow : public QQuickView
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit MainWindow(QWindow *parent);

    static MainWindow *instance() { return instance_; }
    static MainWindow *create(QQmlEngine *qmlEngine, QJSEngine *)
    {
        // The instance has to exist before it is used. We cannot replace it.
        Q_ASSERT(instance_);

        // The engine has to have the same thread affinity as the singleton.
        Q_ASSERT(qmlEngine->thread() == instance_->thread());

        // There can only be one engine accessing the singleton.
        static QJSEngine *s_engine = nullptr;
        if (s_engine)
            Q_ASSERT(qmlEngine == s_engine);
        else
            s_engine = qmlEngine;

        QJSEngine::setObjectOwnership(instance_, QJSEngine::CppOwnership);
        return instance_;
    }

    void saveCurrentWindowSize();

    void openJoinRoomDialog(std::function<void(const QString &room_id)> callback);

    MxcImageProvider *imageProvider() { return imgProvider; }

    //! Show the chat page and start communicating with the given access token.
    void showChatPage();

#ifdef NHEKO_DBUS_SYS
    bool dbusAvailable() const { return dbusAvailable_; }
#endif

    Q_INVOKABLE void addPerRoomWindow(const QString &room, QWindow *window);
    Q_INVOKABLE void removePerRoomWindow(const QString &room, QWindow *window);
    QWindow *windowForRoom(const QString &room);
    QString focusedRoom() const;

protected:
    void closeEvent(QCloseEvent *event) override;
    // HACK: https://bugreports.qt.io/browse/QTBUG-83972, qtwayland cannot auto hide menu
    void mousePressEvent(QMouseEvent *) override;

private slots:
    //! Handle interaction with the tray icon.
    void iconActivated(QSystemTrayIcon::ActivationReason reason);

    virtual void setWindowTitle(int notificationCount);

signals:
    // HACK: https://bugreports.qt.io/browse/QTBUG-83972, qtwayland cannot auto hide menu
    void hideMenu();
    void reload();
    void secretsChanged();

    void showNotification(QString msg);

    void switchToChatPage();
    void switchToWelcomePage();
    void switchToLoginPage(QString error);

private:
    void showDialog(QWidget *dialog);
    bool hasActiveUser();
    void restoreWindowSize();
    //! Check if the current page supports the "minimize to tray" functionality.
    bool pageSupportsTray() const;

    void registerQmlTypes();

    static MainWindow *instance_;

    //! The initial welcome screen.
    WelcomePage *welcome_page_;
    //! The register page.
    RegisterPage *register_page_;
    //! The main chat area.
    ChatPage *chat_page_;
    QSharedPointer<UserSettings> userSettings_;
    //! Tray icon that shows the unread message count.
    TrayIcon *trayIcon_;
    Dock *dock_;

    MxcImageProvider *imgProvider = nullptr;

    QMultiHash<QString, QWindow *> roomWindows_;

#ifdef NHEKO_DBUS_SYS
    bool dbusAvailable_{false};
#endif
};
