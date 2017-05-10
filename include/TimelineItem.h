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

#ifndef HISTORY_VIEW_ITEM_H
#define HISTORY_VIEW_ITEM_H

#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>

#include "ImageItem.h"
#include "Sync.h"

#include "Image.h"
#include "MessageEvent.h"
#include "Notice.h"
#include "Text.h"

namespace events = matrix::events;
namespace msgs = matrix::events::messages;

class TimelineItem : public QWidget
{
	Q_OBJECT
public:
	TimelineItem(const events::MessageEvent<msgs::Notice> &e, bool with_sender, const QString &color, QWidget *parent = 0);
	TimelineItem(const events::MessageEvent<msgs::Text> &e, bool with_sender, const QString &color, QWidget *parent = 0);

	// For local messages.
	TimelineItem(const QString &userid, const QString &color, QString body, QWidget *parent = 0);
	TimelineItem(QString body, QWidget *parent = 0);

	TimelineItem(ImageItem *img, const events::MessageEvent<msgs::Image> &e, const QString &color, QWidget *parent);
	TimelineItem(ImageItem *img, const events::MessageEvent<msgs::Image> &e, QWidget *parent);

	~TimelineItem();

private:
	void generateBody(const QString &body);
	void generateBody(const QString &userid, const QString &color, const QString &body);
	void generateTimestamp(const QDateTime &time);

	QString replaceEmoji(const QString &body);

	void setupLayout();

	QHBoxLayout *top_layout_;

	QLabel *time_label_;
	QLabel *content_label_;
};

#endif  // HISTORY_VIEW_ITEM_H
