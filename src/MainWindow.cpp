// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QMessageBox>

#include <mtx/events/collections.hpp>
#include <mtx/requests.hpp>
#include <mtx/responses/login.hpp>

#include "BlurhashProvider.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "ColorImageProvider.h"
#include "Config.h"
#include "JdenticonProvider.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "MxcImageProvider.h"
#include "TrayIcon.h"
#include "UserSettingsPage.h"
#include "Utils.h"
#include "dock/Dock.h"
#include "encryption/DeviceVerificationFlow.h"
#include "timeline/TimelineViewManager.h"
#include "ui/Theme.h"
#include "voip/CallManager.h"
#include "voip/WebRTCSession.h"

#ifdef NHEKO_DBUS_SYS
#include "dbus/NhekoDBusApi.h"
#endif

MainWindow *MainWindow::instance_ = nullptr;

MainWindow::MainWindow(QWindow *parent)
  : QQuickView(parent)
  , userSettings_{UserSettings::instance()}
{
    instance_ = this;

    MainWindow::setWindowTitle(0);
    setObjectName(QStringLiteral("MainWindow"));
    setResizeMode(QQuickView::SizeRootObjectToView);
    setMinimumHeight(conf::window::minHeight);
    setMinimumWidth(conf::window::minWidth);
    restoreWindowSize();

    chat_page_ = new ChatPage(userSettings_, this);
    registerQmlTypes();

    setColor(Theme::paletteFromTheme(userSettings_->theme()).window().color());
    setSource(QUrl(QStringLiteral("qrc:///resources/qml/Root.qml")));

    trayIcon_ = new TrayIcon(QStringLiteral(":/logos/nheko.svg"), this);

    connect(chat_page_, &ChatPage::closing, this, [this] { switchToLoginPage(""); });
    connect(chat_page_, &ChatPage::unreadMessages, this, &MainWindow::setWindowTitle);
    connect(chat_page_, SIGNAL(unreadMessages(int)), trayIcon_, SLOT(setUnreadCount(int)));
    connect(chat_page_, &ChatPage::showLoginPage, this, &MainWindow::switchToLoginPage);
    connect(chat_page_, &ChatPage::showNotification, this, &MainWindow::showNotification);

    connect(userSettings_.get(), &UserSettings::trayChanged, trayIcon_, &TrayIcon::setVisible);
    connect(trayIcon_,
            SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this,
            SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

    trayIcon_->setVisible(userSettings_->tray());
    dock_ = new Dock(this);
    connect(chat_page_, SIGNAL(unreadMessages(int)), dock_, SLOT(setUnreadCount(int)));

    // load cache on event loop
    QTimer::singleShot(0, this, [this] {
        if (hasActiveUser()) {
            QString token       = userSettings_->accessToken();
            QString home_server = userSettings_->homeserver();
            QString user_id     = userSettings_->userId();
            QString device_id   = userSettings_->deviceId();

            http::client()->set_access_token(token.toStdString());
            http::client()->set_server(home_server.toStdString());
            http::client()->set_device_id(device_id.toStdString());

            try {
                using namespace mtx::identifiers;
                http::client()->set_user(parse<User>(user_id.toStdString()));
            } catch (const std::invalid_argument &) {
                nhlog::ui()->critical("bootstrapped with invalid user_id: {}",
                                      user_id.toStdString());
            }

            nhlog::ui()->info("User already signed in, showing chat page");
            showChatPage();
        }
    });
}

void
MainWindow::registerQmlTypes()
{
    imgProvider = new MxcImageProvider();
    engine()->addImageProvider(QStringLiteral("MxcImage"), imgProvider);
    engine()->addImageProvider(QStringLiteral("colorimage"), new ColorImageProvider());
    engine()->addImageProvider(QStringLiteral("blurhash"), new BlurhashProvider());
    if (JdenticonProvider::isAvailable())
        engine()->addImageProvider(QStringLiteral("jdenticon"), new JdenticonProvider());

    QObject::connect(engine(), &QQmlEngine::quit, &QGuiApplication::quit);

#ifdef NHEKO_DBUS_SYS
    if (UserSettings::instance()->exposeDBusApi()) {
        if (QDBusConnection::sessionBus().isConnected() &&
            QDBusConnection::sessionBus().registerService(NHEKO_DBUS_SERVICE_NAME)) {
            nheko::dbus::init();
            nhlog::ui()->info("Initialized D-Bus");
            dbusAvailable_ = true;
        } else
            nhlog::ui()->warn("Could not connect to D-Bus!");
    }
#endif
}

void
MainWindow::setWindowTitle(int notificationCount)
{
    QString name = QStringLiteral("nheko");

    if (!userSettings_.data()->profile().isEmpty())
        name += " | " + userSettings_.data()->profile();
    if (notificationCount > 0) {
        name.append(QString{QStringLiteral(" (%1)")}.arg(notificationCount));
    }
    QQuickView::setTitle(name);
}

// HACK: https://bugreports.qt.io/browse/QTBUG-83972, qtwayland cannot auto hide menu
void
MainWindow::mousePressEvent(QMouseEvent *event)
{
#if defined(Q_OS_LINUX)
    if (QGuiApplication::platformName() == "wayland") {
        emit hideMenu();
    }
#endif
    return QQuickView::mousePressEvent(event);
}

void
MainWindow::restoreWindowSize()
{
    int savedWidth  = userSettings_->qsettings()->value(QStringLiteral("window/width")).toInt();
    int savedheight = userSettings_->qsettings()->value(QStringLiteral("window/height")).toInt();

    nhlog::ui()->info("Restoring window size {}x{}", savedWidth, savedheight);

    if (savedWidth == 0 || savedheight == 0)
        resize(conf::window::width, conf::window::height);
    else
        resize(savedWidth, savedheight);
}

void
MainWindow::saveCurrentWindowSize()
{
    auto settings = userSettings_->qsettings();
    QSize current = size();

    settings->setValue(QStringLiteral("window/width"), current.width());
    settings->setValue(QStringLiteral("window/height"), current.height());
}

void
MainWindow::showChatPage()
{
    auto userid     = QString::fromStdString(http::client()->user_id().to_string());
    auto device_id  = QString::fromStdString(http::client()->device_id());
    auto homeserver = QString::fromStdString(http::client()->server_url());
    auto token      = QString::fromStdString(http::client()->access_token());

    userSettings_.data()->setUserId(userid);
    userSettings_.data()->setAccessToken(token);
    userSettings_.data()->setDeviceId(device_id);
    userSettings_.data()->setHomeserver(homeserver);

    chat_page_->bootstrap(userid, homeserver, token);
    connect(cache::client(), &Cache::databaseReady, this, &MainWindow::secretsChanged);
    connect(cache::client(), &Cache::secretChanged, this, &MainWindow::secretsChanged);

    emit reload();
    nhlog::ui()->info("Switching to chat page");
    emit switchToChatPage();
}

bool
NhekoFixupPaletteEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    // Workaround for the QGuiApplication palette not being applied to toplevel windows for some
    // reason?!?
    if (event->type() == QEvent::ChildAdded &&
        obj->metaObject()->className() == QStringLiteral("QQuickRootItem")) {
        for (const auto window : QGuiApplication::topLevelWindows()) {
            if (window->property("posted").isValid())
                continue;
            QGuiApplication::postEvent(window, new QEvent(QEvent::ApplicationPaletteChange));
            window->setProperty("posted", true);
        }
    }
    return false;
}

void
MainWindow::closeEvent(QCloseEvent *event)
{
    if (WebRTCSession::instance().state() != webrtc::State::DISCONNECTED) {
        if (QMessageBox::question(
              nullptr, QStringLiteral("nheko"), QStringLiteral("A call is in progress. Quit?")) !=
            QMessageBox::Yes) {
            event->ignore();
            return;
        }
    }

    if (!qApp->isSavingSession() && isVisible() && pageSupportsTray() && userSettings_->tray()) {
        event->ignore();
        hide();
        return;
    }
}

void
MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
        if (!isVisible()) {
            show();
        } else {
            hide();
        }
        break;
    default:
        break;
    }
}

bool
MainWindow::hasActiveUser()
{
    auto settings = userSettings_->qsettings();
    QString prefix;
    if (userSettings_->profile() != QLatin1String(""))
        prefix = "profile/" + userSettings_->profile() + "/";

    return !settings->value(prefix + "auth/access_token").toString().isEmpty() &&
           !settings->value(prefix + "auth/home_server").toString().isEmpty() &&
           !settings->value(prefix + "auth/user_id").toString().isEmpty();
}

bool
MainWindow::pageSupportsTray() const
{
    return !http::client()->access_token().empty();
}

inline void
MainWindow::showDialog(QWidget *dialog)
{
    dialog->setWindowFlags(Qt::WindowType::Dialog | Qt::WindowType::WindowCloseButtonHint |
                           Qt::WindowType::WindowTitleHint);
    dialog->raise();
    dialog->show();
    utils::centerWidget(dialog, this);
    dialog->window()->windowHandle()->setTransientParent(this);
}

void
MainWindow::addPerRoomWindow(const QString &room, QWindow *window)
{
    roomWindows_.insert(room, window);
}
void
MainWindow::removePerRoomWindow(const QString &room, QWindow *window)
{
    roomWindows_.remove(room, window);
}
QWindow *
MainWindow::windowForRoom(const QString &room)
{
    auto currMainWindowRoom = ChatPage::instance()->timelineManager()->rooms()->currentRoom();
    if ((currMainWindowRoom && currMainWindowRoom->roomId() == room) ||
        ChatPage::instance()->timelineManager()->rooms()->currentRoomPreview().roomid_ == room)
        return this;
    else if (auto res = roomWindows_.find(room); res != roomWindows_.end())
        return res.value();
    return nullptr;
}

QString
MainWindow::focusedRoom() const
{
    auto focus = QGuiApplication::focusWindow();
    if (!focus)
        return {};

    if (focus == this) {
        auto currMainWindowRoom = ChatPage::instance()->timelineManager()->rooms()->currentRoom();
        if (currMainWindowRoom)
            return currMainWindowRoom->roomId();
        else
            return ChatPage::instance()->timelineManager()->rooms()->currentRoomPreview().roomid_;
    }

    auto i = roomWindows_.constBegin();
    while (i != roomWindows_.constEnd()) {
        if (i.value() == focus)
            return i.key();
        ++i;
    }

    return nullptr;
}
