// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QMessageBox>

#include <mtx/requests.hpp>
#include <mtx/responses/login.hpp>

#include "AliasEditModel.h"
#include "BlurhashProvider.h"
#include "Cache.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "Clipboard.h"
#include "ColorImageProvider.h"
#include "CombinedImagePackModel.h"
#include "CompletionProxyModel.h"
#include "Config.h"
#include "EventAccessors.h"
#include "ImagePackListModel.h"
#include "InviteesModel.h"
#include "JdenticonProvider.h"
#include "Logging.h"
#include "LoginPage.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "MemberList.h"
#include "MxcImageProvider.h"
#include "PowerlevelsEditModels.h"
#include "ReadReceiptsModel.h"
#include "RegisterPage.h"
#include "RoomDirectoryModel.h"
#include "RoomsModel.h"
#include "SingleImagePackModel.h"
#include "TrayIcon.h"
#include "UserSettingsPage.h"
#include "UsersModel.h"
#include "Utils.h"
#include "dock/Dock.h"
#include "emoji/EmojiModel.h"
#include "emoji/Provider.h"
#include "encryption/DeviceVerificationFlow.h"
#include "encryption/SelfVerificationStatus.h"
#include "timeline/DelegateChooser.h"
#include "timeline/TimelineViewManager.h"
#include "ui/HiddenEvents.h"
#include "ui/MxcAnimatedImage.h"
#include "ui/MxcMediaProxy.h"
#include "ui/NhekoCursorShape.h"
#include "ui/NhekoDropArea.h"
#include "ui/NhekoEventObserver.h"
#include "ui/NhekoGlobalObject.h"
#include "ui/RoomSummary.h"
#include "ui/UIA.h"
#include "voip/CallManager.h"
#include "voip/WebRTCSession.h"

#ifdef NHEKO_DBUS_SYS
#include "dbus/NhekoDBusApi.h"
#endif

Q_DECLARE_METATYPE(mtx::events::collections::TimelineEvents)
Q_DECLARE_METATYPE(std::vector<DeviceInfo>)
Q_DECLARE_METATYPE(std::vector<mtx::responses::PublicRoomsChunk>)
Q_DECLARE_METATYPE(mtx::responses::PublicRoom)
Q_DECLARE_METATYPE(mtx::responses::Profile)

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
    setSource(QUrl(QStringLiteral("qrc:///qml/Root.qml")));

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
    qRegisterMetaType<mtx::events::msg::KeyVerificationAccept>();
    qRegisterMetaType<mtx::events::msg::KeyVerificationCancel>();
    qRegisterMetaType<mtx::events::msg::KeyVerificationDone>();
    qRegisterMetaType<mtx::events::msg::KeyVerificationKey>();
    qRegisterMetaType<mtx::events::msg::KeyVerificationMac>();
    qRegisterMetaType<mtx::events::msg::KeyVerificationReady>();
    qRegisterMetaType<mtx::events::msg::KeyVerificationRequest>();
    qRegisterMetaType<mtx::events::msg::KeyVerificationStart>();
    qRegisterMetaType<mtx::responses::PublicRoom>();
    qRegisterMetaType<mtx::responses::Profile>();
    qRegisterMetaType<CombinedImagePackModel *>();
    qRegisterMetaType<mtx::events::collections::TimelineEvents>();
    qRegisterMetaType<std::vector<DeviceInfo>>();

    qRegisterMetaType<std::vector<mtx::responses::PublicRoomsChunk>>();

    qmlRegisterUncreatableMetaObject(qml_mtx_events::staticMetaObject,
                                     "im.nheko",
                                     1,
                                     0,
                                     "MtxEvent",
                                     QStringLiteral("Can't instantiate enum!"));
    qmlRegisterUncreatableMetaObject(
      olm::staticMetaObject, "im.nheko", 1, 0, "Olm", QStringLiteral("Can't instantiate enum!"));
    qmlRegisterUncreatableMetaObject(crypto::staticMetaObject,
                                     "im.nheko",
                                     1,
                                     0,
                                     "Crypto",
                                     QStringLiteral("Can't instantiate enum!"));
    qmlRegisterUncreatableMetaObject(verification::staticMetaObject,
                                     "im.nheko",
                                     1,
                                     0,
                                     "VerificationStatus",
                                     QStringLiteral("Can't instantiate enum!"));

    qmlRegisterType<DelegateChoice>("im.nheko", 1, 0, "DelegateChoice");
    qmlRegisterType<DelegateChooser>("im.nheko", 1, 0, "DelegateChooser");
    qmlRegisterType<NhekoDropArea>("im.nheko", 1, 0, "NhekoDropArea");
    qmlRegisterType<NhekoCursorShape>("im.nheko", 1, 0, "CursorShape");
    qmlRegisterType<NhekoEventObserver>("im.nheko", 1, 0, "EventObserver");
    qmlRegisterType<MxcAnimatedImage>("im.nheko", 1, 0, "MxcAnimatedImage");
    qmlRegisterType<MxcMediaProxy>("im.nheko", 1, 0, "MxcMedia");
    qmlRegisterType<RoomDirectoryModel>("im.nheko", 1, 0, "RoomDirectoryModel");
    qmlRegisterType<LoginPage>("im.nheko", 1, 0, "Login");
    qmlRegisterType<RegisterPage>("im.nheko", 1, 0, "Registration");
    qmlRegisterType<HiddenEvents>("im.nheko", 1, 0, "HiddenEvents");
    qmlRegisterUncreatableType<RoomSummary>(
      "im.nheko",
      1,
      0,
      "RoomSummary",
      QStringLiteral("Please use joinRoom to create a room summary."));
    qmlRegisterUncreatableType<AliasEditingModel>(
      "im.nheko",
      1,
      0,
      "AliasEditingModel",
      QStringLiteral("Please use editAliases to create the models"));

    qmlRegisterUncreatableType<PowerlevelEditingModels>(
      "im.nheko",
      1,
      0,
      "PowerlevelEditingModels",
      QStringLiteral("Please use editPowerlevels to create the models"));
    qmlRegisterUncreatableType<DeviceVerificationFlow>(
      "im.nheko",
      1,
      0,
      "DeviceVerificationFlow",
      QStringLiteral("Can't create verification flow from QML!"));
    qmlRegisterUncreatableType<UserProfile>(
      "im.nheko",
      1,
      0,
      "UserProfileModel",
      QStringLiteral("UserProfile needs to be instantiated on the C++ side"));
    qmlRegisterUncreatableType<MemberList>(
      "im.nheko",
      1,
      0,
      "MemberList",
      QStringLiteral("MemberList needs to be instantiated on the C++ side"));
    qmlRegisterUncreatableType<RoomSettings>(
      "im.nheko",
      1,
      0,
      "RoomSettingsModel",
      QStringLiteral("Room Settings needs to be instantiated on the C++ side"));
    qmlRegisterUncreatableType<TimelineModel>(
      "im.nheko", 1, 0, "Room", QStringLiteral("Room needs to be instantiated on the C++ side"));
    qmlRegisterUncreatableType<ImagePackListModel>(
      "im.nheko",
      1,
      0,
      "ImagePackListModel",
      QStringLiteral("ImagePackListModel needs to be instantiated on the C++ side"));
    qmlRegisterUncreatableType<SingleImagePackModel>(
      "im.nheko",
      1,
      0,
      "SingleImagePackModel",
      QStringLiteral("SingleImagePackModel needs to be instantiated on the C++ side"));
    qmlRegisterUncreatableType<InviteesModel>(
      "im.nheko",
      1,
      0,
      "InviteesModel",
      QStringLiteral("InviteesModel needs to be instantiated on the C++ side"));
    qmlRegisterUncreatableType<ReadReceiptsProxy>(
      "im.nheko",
      1,
      0,
      "ReadReceiptsProxy",
      QStringLiteral("ReadReceiptsProxy needs to be instantiated on the C++ side"));

    qmlRegisterSingletonType<Clipboard>(
      "im.nheko", 1, 0, "Clipboard", [](QQmlEngine *, QJSEngine *) -> QObject * {
          return new Clipboard();
      });
    qmlRegisterSingletonType<Nheko>(
      "im.nheko", 1, 0, "Nheko", [](QQmlEngine *, QJSEngine *) -> QObject * {
          return new Nheko();
      });
    qmlRegisterSingletonType<UserSettingsModel>(
      "im.nheko", 1, 0, "UserSettingsModel", [](QQmlEngine *, QJSEngine *) -> QObject * {
          return new UserSettingsModel();
      });

    qmlRegisterSingletonInstance("im.nheko", 1, 0, "Settings", userSettings_.data());

    qRegisterMetaType<mtx::events::collections::TimelineEvents>();
    qRegisterMetaType<std::vector<DeviceInfo>>();

    qmlRegisterUncreatableType<FilteredCommunitiesModel>(
      "im.nheko",
      1,
      0,
      "FilteredCommunitiesModel",
      QStringLiteral("Use Communities.filtered() to create a FilteredCommunitiesModel"));

    qmlRegisterType<emoji::EmojiModel>("im.nheko.EmojiModel", 1, 0, "EmojiModel");
    qmlRegisterUncreatableType<emoji::Emoji>(
      "im.nheko.EmojiModel", 1, 0, "Emoji", QStringLiteral("Used by emoji models"));
    qmlRegisterUncreatableType<MediaUpload>(
      "im.nheko", 1, 0, "MediaUpload", QStringLiteral("MediaUploads can not be created in Qml"));
    qmlRegisterUncreatableMetaObject(emoji::staticMetaObject,
                                     "im.nheko.EmojiModel",
                                     1,
                                     0,
                                     "EmojiCategory",
                                     QStringLiteral("Error: Only enums"));

    qmlRegisterType<RoomDirectoryModel>("im.nheko", 1, 0, "RoomDirectoryModel");

    qmlRegisterSingletonType<SelfVerificationStatus>(
      "im.nheko", 1, 0, "SelfVerificationStatus", [](QQmlEngine *, QJSEngine *) -> QObject * {
          auto ptr = new SelfVerificationStatus();
          QObject::connect(ChatPage::instance(),
                           &ChatPage::initializeEmptyViews,
                           ptr,
                           &SelfVerificationStatus::invalidate);
          return ptr;
      });
    qmlRegisterSingletonInstance("im.nheko", 1, 0, "MainWindow", this);
    qmlRegisterSingletonInstance("im.nheko", 1, 0, "UIA", UIA::instance());
    qmlRegisterSingletonInstance(
      "im.nheko", 1, 0, "CallManager", ChatPage::instance()->callManager());

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

bool
MainWindow::event(QEvent *event)
{
    auto type = event->type();

    if (type == QEvent::Close) {
        closeEvent(static_cast<QCloseEvent *>(event));
    }

    return QQuickView::event(event);
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

    return settings->contains(prefix + "auth/access_token") &&
           settings->contains(prefix + "auth/home_server") &&
           settings->contains(prefix + "auth/user_id");
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
