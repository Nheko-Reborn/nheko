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

#include <QAbstractItemView>
#include <QCompleter>
#include <QDebug>
#include <QStringListModel>
#include <QTimer>

#include "QuickSwitcher.h"

RoomSearchInput::RoomSearchInput(QWidget* parent)
    : TextField(parent)
{
}

bool RoomSearchInput::focusNextPrevChild(bool next)
{
	Q_UNUSED(next);

	return false;
}

void RoomSearchInput::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Tab) {
		auto completer = this->completer();

		if (completer) {
			// Enable the current item if its valid.
			completer->popup()->setCurrentIndex(completer->currentIndex());

			if (!completer->setCurrentRow(completer->currentRow() + 1))
				completer->setCurrentRow(0);
		}

		event->accept();
		return;
	}

	TextField::keyPressEvent(event);
}

QuickSwitcher::QuickSwitcher(QWidget* parent)
    : QFrame(parent)
{
	setMaximumWidth(400);
	setStyleSheet("background-color: #f9f9f9");

	QFont font;
	font.setPixelSize(20);

	roomSearch_ = new RoomSearchInput(this);
	roomSearch_->setFont(font);
	roomSearch_->setPlaceholderText(tr("Find a room..."));

	QStringList wordList;
	QCompleter* completer = new QCompleter(wordList, this);
	completer->setCaseSensitivity(Qt::CaseInsensitive);

	roomSearch_->setCompleter(completer);

	topLayout_ = new QVBoxLayout(this);
	topLayout_->setMargin(20);
	topLayout_->setSpacing(0);
	topLayout_->addWidget(roomSearch_);

	connect(roomSearch_, &QLineEdit::returnPressed, this, [=]() {
		emit closing();
		emit roomSelected(rooms_[this->roomSearch_->text().trimmed()]);

		roomSearch_->clear();
	});
}

void QuickSwitcher::setRoomList(const QMap<QString, QString>& rooms)
{
	rooms_ = rooms;

	QStringList search_items = rooms.keys();

	if (!roomSearch_->completer())
		return;

	roomSearch_->completer()->setModel(new QStringListModel(search_items));
}

void QuickSwitcher::showEvent(QShowEvent*)
{
	roomSearch_->setFocus();
}

void QuickSwitcher::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Escape) {
		roomSearch_->clear();
		event->accept();
		emit closing();
	}
}
