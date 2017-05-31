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

#pragma once

#include <QAction>
#include <QDebug>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QPaintEvent>
#include <QSharedPointer>
#include <QVBoxLayout>
#include <QWidget>

#include "Avatar.h"
#include "FlatButton.h"
#include "Menu.h"
#include "RoomSettings.h"

class TopRoomBar : public QWidget
{
	Q_OBJECT
public:
	TopRoomBar(QWidget *parent = 0);
	~TopRoomBar();

	inline void updateRoomAvatar(const QImage &avatar_image);
	inline void updateRoomAvatar(const QIcon &icon);
	inline void updateRoomName(const QString &name);
	inline void updateRoomTopic(const QString &topic);
	void updateRoomAvatarFromName(const QString &name);
	void setRoomSettings(QSharedPointer<RoomSettings> settings);

	void reset();

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	QHBoxLayout *top_layout_;
	QVBoxLayout *text_layout_;

	QLabel *name_label_;
	QLabel *topic_label_;

	QSharedPointer<RoomSettings> roomSettings_;

	QMenu *menu_;
	QAction *toggleNotifications_;

	FlatButton *settingsBtn_;

	Avatar *avatar_;

	int buttonSize_;
};

inline void TopRoomBar::updateRoomAvatar(const QImage &avatar_image)
{
	avatar_->setImage(avatar_image);
}

inline void TopRoomBar::updateRoomAvatar(const QIcon &icon)
{
	avatar_->setIcon(icon);
}

inline void TopRoomBar::updateRoomName(const QString &name)
{
	name_label_->setText(name);
}

inline void TopRoomBar::updateRoomTopic(const QString &topic)
{
	topic_label_->setText(topic);
}
