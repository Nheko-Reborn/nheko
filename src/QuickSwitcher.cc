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

#include <QCompleter>
#include <QStringListModel>
#include <QTimer>

#include "QuickSwitcher.h"

RoomSearchInput::RoomSearchInput(QWidget *parent)
  : TextField(parent)
{}

bool
RoomSearchInput::focusNextPrevChild(bool next)
{
        Q_UNUSED(next);

        return false;
}

void
RoomSearchInput::keyPressEvent(QKeyEvent *event)
{
        if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Down) {
                emit selectNextCompletion();
                event->accept();
                return;
        } else if (event->key() == Qt::Key_Up) {
                emit selectPreviousCompletion();
                event->accept();
                return;
        }

        TextField::keyPressEvent(event);
}

void
RoomSearchInput::hideEvent(QHideEvent *event)
{
        emit hiding();
        TextField::hideEvent(event);
}

QuickSwitcher::QuickSwitcher(QWidget *parent)
  : QFrame(parent)
{
        setMaximumWidth(450);
        setStyleSheet("background-color: white");

        QFont font;
        font.setPixelSize(20);

        roomSearch_ = new RoomSearchInput(this);
        roomSearch_->setFont(font);
        roomSearch_->setPlaceholderText(tr("Find a room..."));

        completer_ = new QCompleter();
        completer_->setCaseSensitivity(Qt::CaseInsensitive);
        completer_->setCompletionMode(QCompleter::PopupCompletion);
        completer_->setWidget(this);

        topLayout_ = new QVBoxLayout(this);
        topLayout_->addWidget(roomSearch_);

        connect(completer_, SIGNAL(highlighted(QString)), roomSearch_, SLOT(setText(QString)));
        connect(roomSearch_, &QLineEdit::textEdited, this, [=](const QString &prefix) {
                if (prefix.isEmpty()) {
                        completer_->popup()->hide();
                        selection_ = -1;
                        return;
                }

                if (prefix != completer_->completionPrefix()) {
                        completer_->setCompletionPrefix(prefix);
                        selection_ = -1;
                }

                completer_->popup()->setWindowFlags(completer_->popup()->windowFlags() |
                                                    Qt::ToolTip | Qt::NoDropShadowWindowHint);
                completer_->popup()->setAttribute(Qt::WA_ShowWithoutActivating);
                completer_->complete();
        });

        connect(roomSearch_, &RoomSearchInput::selectNextCompletion, this, [=]() {
                selection_ += 1;

                if (!completer_->setCurrentRow(selection_)) {
                        selection_ = 0;
                        completer_->setCurrentRow(selection_);
                }

                completer_->popup()->setCurrentIndex(completer_->currentIndex());
        });

        connect(roomSearch_, &RoomSearchInput::selectPreviousCompletion, this, [=]() {
                selection_ -= 1;

                if (!completer_->setCurrentRow(selection_)) {
                        selection_ = completer_->completionCount() - 1;
                        completer_->setCurrentRow(selection_);
                }

                completer_->popup()->setCurrentIndex(completer_->currentIndex());
        });

        connect(
          roomSearch_, &RoomSearchInput::hiding, this, [=]() { completer_->popup()->hide(); });
        connect(roomSearch_, &QLineEdit::returnPressed, this, [=]() {
                emit closing();

                QString text("");

                if (selection_ == -1) {
                        completer_->setCurrentRow(0);
                        text = completer_->currentCompletion();
                } else {
                        text = this->roomSearch_->text().trimmed();
                }
                emit roomSelected(rooms_[text]);

                roomSearch_->clear();
        });
}

void
QuickSwitcher::setRoomList(const QMap<QString, QString> &rooms)
{
        rooms_            = rooms;
        QStringList items = rooms.keys();

        completer_->setModel(new QStringListModel(items));
}

void
QuickSwitcher::showEvent(QShowEvent *)
{
        roomSearch_->setFocus();
}

void
QuickSwitcher::keyPressEvent(QKeyEvent *event)
{
        if (event->key() == Qt::Key_Escape) {
                roomSearch_->clear();
                event->accept();
                emit closing();
        }
}
