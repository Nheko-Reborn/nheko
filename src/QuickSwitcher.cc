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
#include <QPainter>
#include <QStringListModel>
#include <QStyleOption>
#include <QTimer>
#include <QtConcurrent>

#include "QuickSwitcher.h"

RoomSearchInput::RoomSearchInput(QWidget *parent)
  : TextField(parent)
{}

void
RoomSearchInput::keyPressEvent(QKeyEvent *event)
{
        switch (event->key()) {
        case Qt::Key_Tab:
        case Qt::Key_Down: {
                emit selectNextCompletion();
                event->accept();
                break;
        }
        case Qt::Key_Backtab:
        case Qt::Key_Up: {
                emit selectPreviousCompletion();
                event->accept();
                break;
        }
        default:
                TextField::keyPressEvent(event);
        }
}

void
RoomSearchInput::hideEvent(QHideEvent *event)
{
        emit hiding();
        TextField::hideEvent(event);
}

QuickSwitcher::QuickSwitcher(QWidget *parent)
  : QWidget(parent)
{
        qRegisterMetaType<std::vector<RoomSearchResult>>();
        setMaximumWidth(450);

        QFont font;
        font.setPixelSize(20);

        roomSearch_ = new RoomSearchInput(this);
        roomSearch_->setFont(font);
        roomSearch_->setPlaceholderText(tr("Search for a room..."));

        topLayout_ = new QVBoxLayout(this);
        topLayout_->addWidget(roomSearch_);

        connect(this,
                &QuickSwitcher::queryResults,
                this,
                [this](const std::vector<RoomSearchResult> &rooms) {
                        auto pos = mapToGlobal(roomSearch_->geometry().bottomLeft());

                        popup_.setFixedWidth(width());
                        popup_.addRooms(rooms);
                        popup_.move(pos.x() - topLayout_->margin(), pos.y() + topLayout_->margin());
                        popup_.show();
                });

        connect(roomSearch_, &QLineEdit::textEdited, this, [this](const QString &query) {
                if (query.isEmpty()) {
                        popup_.hide();
                        return;
                }

                QtConcurrent::run([this, query = query.toLower()]() {
                        try {
                                emit queryResults(
                                  cache::client()->searchRooms(query.toStdString()));
                        } catch (const lmdb::error &e) {
                                qWarning() << "room search failed:" << e.what();
                        }
                });
        });

        connect(roomSearch_,
                &RoomSearchInput::selectNextCompletion,
                &popup_,
                &SuggestionsPopup::selectNextSuggestion);
        connect(roomSearch_,
                &RoomSearchInput::selectPreviousCompletion,
                &popup_,
                &SuggestionsPopup::selectPreviousSuggestion);
        connect(&popup_, &SuggestionsPopup::itemSelected, this, [this](const QString &room_id) {
                reset();
                emit roomSelected(room_id);
        });
        connect(roomSearch_, &RoomSearchInput::hiding, this, [this]() { popup_.hide(); });
        connect(roomSearch_, &QLineEdit::returnPressed, this, [this]() {
                reset();
                popup_.selectHoveredSuggestion<RoomItem>();
        });
}

void
QuickSwitcher::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void
QuickSwitcher::keyPressEvent(QKeyEvent *event)
{
        if (event->key() == Qt::Key_Escape) {
                event->accept();
                reset();
        }
}
