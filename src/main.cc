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
#include <QFontDatabase>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	QCoreApplication::setApplicationName("nheko");
	QCoreApplication::setApplicationVersion("Ωμέγa");
	QCoreApplication::setOrganizationName("Nheko");
	QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

	QFontDatabase::addApplicationFont(":/fonts/fonts/OpenSans/OpenSans-Regular.ttf");
	QFontDatabase::addApplicationFont(":/fonts/fonts/OpenSans/OpenSans-Italic.ttf");
	QFontDatabase::addApplicationFont(":/fonts/fonts/OpenSans/OpenSans-Bold.ttf");
	QFontDatabase::addApplicationFont(":/fonts/fonts/OpenSans/OpenSans-BoldItalic.ttf");
	QFontDatabase::addApplicationFont(":/fonts/fonts/OpenSans/OpenSans-Semibold.ttf");
	QFontDatabase::addApplicationFont(":/fonts/fonts/OpenSans/OpenSans-SemiboldItalic.ttf");
	QFontDatabase::addApplicationFont(":/fonts/fonts/EmojiOne/emojione-android.ttf");

	app.setWindowIcon(QIcon(":/logos/nheko.png"));

	app.setStyleSheet(
		"QScrollBar:vertical { background-color: #f8fbfe; width: 8px; border: none; margin: 2px; }"
		"QScrollBar::handle:vertical { background-color : #d6dde3; }"
		"QScrollBar::add-line:vertical { border: none; background: none; }"
		"QScrollBar::sub-line:vertical { border: none; background: none; }");

	QFont font("Open Sans");
	app.setFont(font);

	MainWindow w;

	// Move the MainWindow to the center
	QRect screenGeometry = QApplication::desktop()->screenGeometry();
	int x = (screenGeometry.width() - w.width()) / 2;
	int y = (screenGeometry.height() - w.height()) / 2;

	w.move(x, y);
	w.show();

	return app.exec();
}
