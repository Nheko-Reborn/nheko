/*
 * nheko Copyright (C) 2017  Konstantinos Sideris <siderisk@auth.gr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QApplication>
#include <QCommandLineParser>
#include <QDesktopWidget>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QLabel>
#include <QLayout>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QPoint>
#include <QSettings>
#include <QStandardPaths>
#include <QTranslator>

#include "Config.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "RunGuard.h"
#include "Utils.h"
#include "version.h"

#if defined(Q_OS_LINUX)
#include <boost/stacktrace.hpp>
#include <signal.h>

void
stacktraceHandler(int signum)
{
        std::signal(signum, SIG_DFL);
        boost::stacktrace::safe_dump_to("./nheko-backtrace.dump");
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
{}

#endif

QPoint
screenCenter(int width, int height)
{
        QRect screenGeometry = QApplication::desktop()->screenGeometry();

        int x = (screenGeometry.width() - width) / 2;
        int y = (screenGeometry.height() - height) / 2;

        return QPoint(x, y);
}

void
createCacheDirectory()
{
        auto dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

        if (!QDir().mkpath(dir)) {
                throw std::runtime_error(
                  ("Unable to create state directory:" + dir).toStdString().c_str());
        }
}

int
main(int argc, char *argv[])
{
        RunGuard guard("run_guard");

        if (!guard.tryToRun()) {
                QApplication a(argc, argv);

                QMessageBox msgBox;
                msgBox.setText("Another instance of Nheko is running");
                msgBox.exec();

                return 0;
        }

#if defined(Q_OS_LINUX) || defined(Q_OS_WIN) || defined(Q_OS_FREEBSD)
        if (qgetenv("QT_SCALE_FACTOR").size() == 0) {
                float factor = utils::scaleFactor();

                if (factor != -1)
                        qputenv("QT_SCALE_FACTOR", QString::number(factor).toUtf8());
        }
#endif

        QApplication app(argc, argv);
        QCoreApplication::setApplicationName("nheko");
        QCoreApplication::setApplicationVersion(nheko::version);
        QCoreApplication::setOrganizationName("nheko");
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

        QCommandLineParser parser;
        parser.addHelpOption();
        parser.addVersionOption();
        parser.process(app);

        QFontDatabase::addApplicationFont(":/fonts/fonts/OpenSans/OpenSans-Regular.ttf");
        QFontDatabase::addApplicationFont(":/fonts/fonts/OpenSans/OpenSans-Italic.ttf");
        QFontDatabase::addApplicationFont(":/fonts/fonts/OpenSans/OpenSans-Bold.ttf");
        QFontDatabase::addApplicationFont(":/fonts/fonts/OpenSans/OpenSans-Semibold.ttf");
        QFontDatabase::addApplicationFont(":/fonts/fonts/EmojiOne/emojione-android.ttf");

        app.setWindowIcon(QIcon(":/logos/nheko.png"));

        http::init();

        createCacheDirectory();

        registerSignalHandlers();

        try {
                nhlog::init(QString("%1/nheko.log")
                              .arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
                              .toStdString());
        } catch (const spdlog::spdlog_ex &ex) {
                std::cout << "Log initialization failed: " << ex.what() << std::endl;
                std::exit(1);
        }

        QSettings settings;

        // Set the default if a value has not been set.
        if (settings.value("font/size").toInt() == 0)
                settings.setValue("font/size", 12);

        QFont font("Open Sans", settings.value("font/size").toInt());
        app.setFont(font);

        QString lang = QLocale::system().name();

        QTranslator qtTranslator;
        qtTranslator.load("qt_" + lang, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
        app.installTranslator(&qtTranslator);

        QTranslator appTranslator;
        appTranslator.load("nheko_" + lang, ":/translations");
        app.installTranslator(&appTranslator);

        MainWindow w;

        // Move the MainWindow to the center
        w.move(screenCenter(w.width(), w.height()));

        if (!settings.value("user/window/start_in_tray", false).toBool() ||
            !settings.value("user/window/tray", true).toBool())
                w.show();

        QObject::connect(&app, &QApplication::aboutToQuit, &w, [&w]() {
                w.saveCurrentWindowSize();
                if (http::client() != nullptr) {
                        nhlog::net()->debug("shutting down all I/O threads & open connections");
                        http::client()->close(true);
                        nhlog::net()->debug("bye");
                }
        });

        nhlog::ui()->info("starting nheko {}", nheko::version);

        return app.exec();
}
