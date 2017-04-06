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

#ifndef ROOMINFOLISTITEM_H
#define ROOMINFOLISTITEM_H

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "Avatar.h"
#include "RippleOverlay.h"
#include "RoomInfo.h"

class RoomInfoListItem : public QWidget
{
	Q_OBJECT

public:
	RoomInfoListItem(RoomInfo info, QWidget *parent = 0);
	~RoomInfoListItem();

	inline bool isPressed();
	inline RoomInfo info();
	inline void setAvatar(const QImage &avatar_image);

signals:
	void clicked(const RoomInfo &info_);

public slots:
	void setPressedState(bool state);

protected:
	void mousePressEvent(QMouseEvent *event) override;

private:
	void setElidedText(QLabel *label, QString text, int width);

	RippleOverlay *ripple_overlay_;

	RoomInfo info_;

	QHBoxLayout *topLayout_;

	QVBoxLayout *avatarLayout_;
	QVBoxLayout *textLayout_;

	QWidget *avatarWidget_;
	QWidget *textWidget_;

	QLabel *roomName_;
	QLabel *roomTopic_;

	Avatar *roomAvatar_;

	QString pressed_style_;
	QString normal_style_;

	bool is_pressed_;
	int max_height_;
};

inline bool RoomInfoListItem::isPressed()
{
	return is_pressed_;
}

inline RoomInfo RoomInfoListItem::info()
{
	return info_;
}

inline void RoomInfoListItem::setAvatar(const QImage &avatar_image)
{
	roomAvatar_->setImage(avatar_image);
}

#endif  // ROOMINFOLISTITEM_H
