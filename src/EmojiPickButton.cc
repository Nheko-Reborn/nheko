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

#include <QDebug>

#include "EmojiPickButton.h"

EmojiPickButton::EmojiPickButton(QWidget *parent)
    : FlatButton(parent)
    , panel_{nullptr}
{
}

void EmojiPickButton::enterEvent(QEvent *e)
{
	Q_UNUSED(e);

	if (panel_ == nullptr) {
		panel_ = new EmojiPanel(this);
		connect(panel_, &EmojiPanel::emojiSelected, this, &EmojiPickButton::emojiSelected);
	}

	QPoint pos(rect().x(), rect().y());
	pos = this->mapToGlobal(pos);

	auto panel_size = panel_->sizeHint();

	auto x = pos.x() - panel_size.width() + horizontal_distance_;
	auto y = pos.y() - panel_size.height() - vertical_distance_;

	panel_->move(x, y);
	panel_->fadeIn();
	panel_->show();
}

void EmojiPickButton::leaveEvent(QEvent *e)
{
	Q_UNUSED(e);

	if (panel_->underMouse())
		return;

	auto pos = QCursor::pos();
	auto panel_geometry = panel_->geometry();
	panel_geometry.adjust(0, 0, 0, vertical_distance_);

	if (panel_geometry.contains(pos))
		return;

	panel_->fadeOut();
}
