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
	QCoreApplication::setApplicationName("nheko");
	QCoreApplication::setApplicationVersion("Ωμέγa");
	QCoreApplication::setOrganizationName("Nheko");

	QFontDatabase::addApplicationFont(":/fonts/OpenSans-Light.ttf");
	QFontDatabase::addApplicationFont(":/fonts/OpenSans-Regular.ttf");
	QFontDatabase::addApplicationFont(":/fonts/OpenSans-Italic.ttf");
	QFontDatabase::addApplicationFont(":/fonts/OpenSans-Bold.ttf");
	QFontDatabase::addApplicationFont(":/fonts/OpenSans-BoldItalic.ttf");
	QFontDatabase::addApplicationFont(":/fonts/OpenSans-Semibold.ttf");
	QFontDatabase::addApplicationFont(":/fonts/OpenSans-SemiboldItalic.ttf");
	QFontDatabase::addApplicationFont(":/fonts/OpenSans-ExtraBold.ttf");
	QFontDatabase::addApplicationFont(":/fonts/OpenSans-ExtraBoldItalic.ttf");

	QApplication app(argc, argv);

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
