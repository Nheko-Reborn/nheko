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
#include <QDesktopWidget>
#include <QFile>
#include <QFontDatabase>
#include <QLabel>
#include <QLayout>
#include <QLibraryInfo>
#include <QPalette>
#include <QPoint>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTranslator>

#include "Config.h"
#include "Logging.hpp"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "RaisedButton.h"
#include "RunGuard.h"
#include "version.hpp"

QPoint
screenCenter(int width, int height)
{
        QRect screenGeometry = QApplication::desktop()->screenGeometry();

        int x = (screenGeometry.width() - width) / 2;
        int y = (screenGeometry.height() - height) / 2;

        return QPoint(x, y);
}

int
main(int argc, char *argv[])
{
        RunGuard guard("run_guard");

        if (!guard.tryToRun()) {
                QApplication a(argc, argv);

                QFont font;
                font.setPointSize(15);
                font.setWeight(60);

                QWidget widget;
                QVBoxLayout layout(&widget);
                layout.setContentsMargins(20, 10, 20, 20);
                layout.setSpacing(0);

                QHBoxLayout btnLayout;

                QLabel msg("Another instance of nheko is currently running.");
                msg.setWordWrap(true);
                msg.setFont(font);

                QPalette pal;

                RaisedButton submitBtn("OK");
                submitBtn.setBackgroundColor(pal.color(QPalette::Button));
                submitBtn.setForegroundColor(pal.color(QPalette::ButtonText));
                submitBtn.setMinimumSize(120, 35);
                submitBtn.setFontSize(conf::btn::fontSize);
                submitBtn.setCornerRadius(conf::btn::cornerRadius);

                btnLayout.addStretch(1);
                btnLayout.addWidget(&submitBtn);

                layout.addWidget(&msg);
                layout.addLayout(&btnLayout);

                widget.setFixedSize(480, 180);
                widget.move(screenCenter(widget.width(), widget.height()));
                widget.show();

                QObject::connect(&submitBtn, &QPushButton::clicked, &widget, &QWidget::close);

                return a.exec();
        }

        QCoreApplication::setApplicationName("nheko");
        QCoreApplication::setApplicationVersion(nheko::version);
        QCoreApplication::setOrganizationName("nheko");
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

        QApplication app(argc, argv);

        QFontDatabase::addApplicationFont(":/fonts/fonts/OpenSans/OpenSans-Regular.ttf");
        QFontDatabase::addApplicationFont(":/fonts/fonts/OpenSans/OpenSans-Italic.ttf");
        QFontDatabase::addApplicationFont(":/fonts/fonts/OpenSans/OpenSans-Bold.ttf");
        QFontDatabase::addApplicationFont(":/fonts/fonts/OpenSans/OpenSans-Semibold.ttf");
        QFontDatabase::addApplicationFont(":/fonts/fonts/EmojiOne/emojione-android.ttf");

        app.setWindowIcon(QIcon(":/logos/nheko.png"));

        http::init();

        try {
                log::init(QString("%1/nheko.log")
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

        QObject::connect(&app, &QApplication::aboutToQuit, &w, &MainWindow::saveCurrentWindowSize);

        log::main()->info("starting nheko {}", nheko::version);

        return app.exec();
}
