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

#ifndef EMOJI_PICK_BUTTON_H
#define EMOJI_PICK_BUTTON_H

#include <QEvent>
#include <QWidget>

#include "EmojiPanel.h"
#include "FlatButton.h"

class EmojiPickButton : public FlatButton
{
	Q_OBJECT
public:
	explicit EmojiPickButton(QWidget *parent = nullptr);

signals:
	void emojiSelected(const QString &emoji);

protected:
	void enterEvent(QEvent *e) override;
	void leaveEvent(QEvent *e) override;

private:
	// Vertical distance from panel's bottom.
	int vertical_distance_ = 10;

	// Horizontal distance from panel's bottom right corner.
	int horizontal_distance_ = 70;

	EmojiPanel *panel_;
};

#endif  // EMOJI_PICK_BUTTON_H
