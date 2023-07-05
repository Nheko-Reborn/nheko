// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

#include <QApplication>
#include <QCommandLineParser>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QLabel>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QPoint>
#include <QQuickView>
#include <QScreen>
#include <QStandardPaths>
#include <QTranslator>

#include "ChatPage.h"
#include "Config.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "Utils.h"
#include "config/nheko.h"
#include "singleapplication.h"

#if defined(Q_OS_MAC)
#include "emoji/MacHelper.h"
#include "notifications/Manager.h"
#endif

#ifdef GSTREAMER_AVAILABLE
#include <QAbstractEventDispatcher>
#include <gst/gst.h>

#include "voip/CallDevices.h"
#endif

#ifdef QML_DEBUGGING
#include <QQmlDebuggingEnabler>
QQmlDebuggingEnabler enabler;
#endif

#if HAVE_BACKTRACE_SYMBOLS_FD
#include <csignal>
#include <execinfo.h>
#include <fcntl.h>
#include <unistd.h>

void
stacktraceHandler(int signum)
{
    std::signal(signum, SIG_DFL);

    // boost::stacktrace::safe_dump_to("./nheko-backtrace.dump");

    // see
    // https://stackoverflow.com/questions/77005/how-to-automatically-generate-a-stacktrace-when-my-program-crashes/77336#77336
    void *array[50];
    int size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 50);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", signum);
    backtrace_symbols_fd(array, size, STDERR_FILENO);

    int file = ::open("/tmp/nheko-crash.dump",
                      O_CREAT | O_WRONLY | O_TRUNC
#if defined(S_IWUSR) && defined(S_IRUSR)
                      ,
                      S_IWUSR | S_IRUSR
#elif defined(S_IWRITE) && defined(S_IREAD)
                      ,
                      S_IWRITE | S_IREAD
#endif
    );
    if (file != -1) {
        constexpr char header[]   = "Error: signal\n";
        [[maybe_unused]] auto ret = write(file, header, std::size(header) - 1);
        backtrace_symbols_fd(array, size, file);
        close(file);
    }

    std::raise(SIGABRT);
}

void
registerSignalHandlers()
{
    std::signal(SIGSEGV, &stacktraceHandler);
    std::signal(SIGABRT, &stacktraceHandler);
}

#else

// No implementation for systems with no stacktrace support.
void
registerSignalHandlers()
{
}

#endif

#if defined(GSTREAMER_AVAILABLE) && (defined(Q_OS_MAC) || defined(Q_OS_WINDOWS))
GMainLoop *gloop = 0;
GThread *gthread = 0;

extern "C"
{
    static gpointer glibMainLoopThreadFunc(gpointer)
    {
        gloop = g_main_loop_new(0, false);
        g_main_loop_run(gloop);
        g_main_loop_unref(gloop);
        gloop = 0;
        return 0;
    }
} // extern "C"
#endif

QPoint
screenCenter(int width, int height)
{
    // Deprecated in 5.13: QRect screenGeometry = QApplication::desktop()->screenGeometry();
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();

    int x = (screenGeometry.width() - width) / 2;
    int y = (screenGeometry.height() - height) / 2;

    return QPoint(x, y);
}

void
createStandardDirectory(QStandardPaths::StandardLocation path)
{
    auto dir = QStandardPaths::writableLocation(path);

    if (!QDir().mkpath(dir)) {
        throw std::runtime_error(("Unable to create state directory:" + dir).toStdString().c_str());
    }
}

int
main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName(QStringLiteral("nheko"));
    QCoreApplication::setApplicationVersion(nheko::version);
    QCoreApplication::setOrganizationName(QStringLiteral("nheko"));
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    // Disable the qml disk cache by default to prevent crashes on updates. See
    // https://github.com/Nheko-Reborn/nheko/issues/1383
    if (qgetenv("NHEKO_ALLOW_QML_DISK_CACHE").size() == 0) {
        qputenv("QML_DISABLE_DISK_CACHE", "1");
    }

    // this needs to be after setting the application name. Or how would we find our settings
    // file then?
#if !defined(Q_OS_MACOS)
    if (qgetenv("QT_SCALE_FACTOR").size() == 0) {
        float factor = utils::scaleFactor();

        if (factor != -1)
            qputenv("QT_SCALE_FACTOR", QString::number(factor).toUtf8());
    }
#endif

    // This is some hacky programming, but it's necessary (AFAIK?) to get the unique config name
    // parsed before the SingleApplication userdata is set.
    QString userdata{QLatin1String("")};
    QString matrixUri;
    for (int i = 1; i < argc; ++i) {
        QString arg{argv[i]};
        if (arg.startsWith(QLatin1String("--profile="))) {
            arg.remove(QStringLiteral("--profile="));
            userdata = arg;
        } else if (arg.startsWith(QLatin1String("--p="))) {
            arg.remove(QStringLiteral("-p="));
            userdata = arg;
        } else if (arg == QLatin1String("--profile") || arg == QLatin1String("-p")) {
            if (i < argc - 1) // if i is less than argc - 1, we still have a parameter
                              // left to process as the name
            {
                ++i; // the next arg is the name, so increment
                userdata = QString{argv[i]};
            }
        } else if (arg.startsWith(QLatin1String("matrix:"))) {
            matrixUri = arg;
        }
    }

    SingleApplication app(argc,
                          argv,
                          true,
                          SingleApplication::Mode::User | SingleApplication::Mode::ExcludeAppPath |
                            SingleApplication::Mode::ExcludeAppVersion |
                            SingleApplication::Mode::SecondaryNotification,
                          100,
                          userdata == QLatin1String("default") ? QLatin1String("") : userdata);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption debugOption(QStringLiteral("debug"),
                                   QObject::tr("Alias for '--log-level trace'."));
    parser.addOption(debugOption);
    QCommandLineOption logLevel(
      QStringList() << QStringLiteral("l") << QStringLiteral("log-level"),
      QObject::tr("Set the global log level, or a comma-separated list of <component>=<level> "
                  "pairs, or both. For example, to set the default log level to 'warn' but "
                  "disable logging for the 'ui' component, pass 'warn,ui=off'. "
                  "levels:{trace,debug,info,warning,error,critical,off} "
                  "components:{crypto,db,mtx,net,qml,ui}"),
      QObject::tr("level"));
    parser.addOption(logLevel);
    QCommandLineOption logType(
      QStringList() << QStringLiteral("L") << QStringLiteral("log-type"),
      QObject::tr("Set the log output type. A comma-separated list is allowed. "
                  "The default is 'file,stderr'. types:{file,stderr,none}"),
      QObject::tr("type"));
    parser.addOption(logType);

    // This option is not actually parsed via Qt due to the need to parse it before the app
    // name is set. It only exists to keep Qt from complaining about the --profile/-p
    // option and thereby crashing the app.
    QCommandLineOption configName(
      QStringList() << QStringLiteral("p") << QStringLiteral("profile"),
      QCoreApplication::tr("Create a unique profile which allows you to log into several "
                           "accounts at the same time and start multiple instances of nheko."),
      QCoreApplication::tr("profile"),
      QCoreApplication::tr("profile name"));
    parser.addOption(configName);

    parser.process(app);

    // This check needs to happen _after_ process(), so that we actually print help for --help when
    // Nheko is already running.
    if (app.isSecondary()) {
        std::cout << "Sending Matrix URL to main application: " << matrixUri.toStdString()
                  << std::endl;
        //  open uri in main instance
        app.sendMessage(matrixUri.toUtf8());
        return 0;
    }

#if !defined(Q_OS_MAC)
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("nheko"), QIcon{":/logos/nheko.png"}));
#endif
#ifdef NHEKO_FLATPAK
    app.setDesktopFileName(QStringLiteral("im.nheko.Nheko"));
#else
    app.setDesktopFileName(QStringLiteral("nheko"));
#endif

    http::init();

    createStandardDirectory(QStandardPaths::CacheLocation);
    createStandardDirectory(QStandardPaths::AppDataLocation);

    registerSignalHandlers();

#if defined(GSTREAMER_AVAILABLE) && (defined(Q_OS_MAC) || defined(Q_OS_WINDOWS))
    // If the version of Qt we're running in does not use GLib, we need to
    // start a GMainLoop so that gstreamer can dispatch events.
    const QMetaObject *mo = QAbstractEventDispatcher::instance(qApp->thread())->metaObject();
    if (gloop == 0 && strcmp(mo->className(), "QEventDispatcherGlib") != 0 &&
        strcmp(mo->superClass()->className(), "QEventDispatcherGlib") != 0) {
        gthread = g_thread_new(0, glibMainLoopThreadFunc, 0);
    }
#endif

    try {
        QString level;
        if (parser.isSet(logLevel)) {
            level = parser.value(logLevel);
        } else if (parser.isSet(debugOption)) {
            level = "trace";
        } else {
            level = qEnvironmentVariable("NHEKO_LOG_LEVEL");
        }

        QStringList targets =
          (parser.isSet(logType) ? parser.value(logType)
                                 : qEnvironmentVariable("NHEKO_LOG_TYPE", "file,stderr"))
            .split(',', Qt::SkipEmptyParts);
        targets.removeAll("none");
        bool to_stderr = bool(targets.removeAll("stderr"));
        QString path   = targets.removeAll("file")
                           ? QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
                             .filePath("nheko.log")
                           : QLatin1String("");
        if (!targets.isEmpty()) {
            std::cerr << "Invalid log type '" << targets.first().toStdString().c_str() << "'"
                      << std::endl;
            std::exit(1);
        }

        nhlog::init(level, path, to_stderr);

    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        std::exit(1);
    }

    auto filter = new NhekoFixupPaletteEventFilter(&app);
    app.installEventFilter(filter);

    if (parser.isSet(configName))
        UserSettings::initialize(parser.value(configName));
    else
        UserSettings::initialize(std::nullopt);

    auto settings = UserSettings::instance().toWeakRef();

    QFont font;
    QString userFontFamily = settings.lock()->font();
    if (!userFontFamily.isEmpty() && userFontFamily != QLatin1String("default")) {
        font.setFamily(userFontFamily);
    }
    font.setPointSizeF(settings.lock()->fontSize());

    app.setFont(font);
    settings.lock()->applyTheme();

    if (QLocale().language() == QLocale::C)
        QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedKingdom));

    QTranslator qtTranslator;
    if (qtTranslator.load(QLocale(),
                          QStringLiteral("qt"),
                          QStringLiteral("_"),
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        app.installTranslator(&qtTranslator);

    QTranslator appTranslator;
    if (appTranslator.load(QLocale(),
                           QStringLiteral("nheko"),
                           QStringLiteral("_"),
                           QStringLiteral(":/translations")))
        app.installTranslator(&appTranslator);

    MainWindow w(nullptr);
    // QQuickView w;

    // Move the MainWindow to the center
    // w.move(screenCenter(w.width(), w.height()));

    if (!(settings.lock()->startInTray() && settings.lock()->tray()))
        w.show();

    QObject::connect(&app, &QApplication::aboutToQuit, &w, [&w]() {
        ChatPage::instance()->removeAllNotifications();
        w.saveCurrentWindowSize();
        if (http::client() != nullptr) {
            nhlog::net()->debug("shutting down all I/O threads & open connections");
            http::client()->close(true);
            nhlog::net()->debug("bye");
        }
    });
    QObject::connect(&app, &SingleApplication::instanceStarted, &w, [&w]() {
        w.show();
        w.raise();
        w.requestActivate();
    });

    // It seems like handling the message in a blocking manner is a no-go. I have no idea how to
    // fix that, so just use a queued connection for now...  (ASAN does not cooperate and just
    // hides the crash D:)
    QObject::connect(
      &app,
      &SingleApplication::receivedMessage,
      ChatPage::instance(),
      [&](quint32, QByteArray message) {
          QString m = QString::fromUtf8(message);
          ChatPage::instance()->handleMatrixUri(m);
      },
      Qt::QueuedConnection);

    QMetaObject::Connection uriConnection;
    if (app.isPrimary() && !matrixUri.isEmpty()) {
        uriConnection = QObject::connect(ChatPage::instance(),
                                         &ChatPage::contentLoaded,
                                         ChatPage::instance(),
                                         [&uriConnection, matrixUri]() {
                                             ChatPage::instance()->handleMatrixUri(matrixUri);
                                             QObject::disconnect(uriConnection);
                                         });
    }
    QDesktopServices::setUrlHandler(
      QStringLiteral("matrix"), ChatPage::instance(), "handleMatrixUri");

#if defined(Q_OS_MAC)
    // Temporary solution for the emoji picker until
    // nheko has a proper menu bar with more functionality.
    MacHelper::initializeMenus();

    // Need to set up notification delegate so users can respond to messages from within the
    // notification itself.
    NotificationsManager::attachToMacNotifCenter();
#endif

    nhlog::ui()->info("starting nheko {}", nheko::version);

    auto returnvalue = app.exec();

#ifdef GSTREAMER_AVAILABLE
    CallDevices::instance().deinit();

    gst_deinit();
#endif

    return returnvalue;
}
