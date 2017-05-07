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

#ifndef HISTORY_VIEW_H
#define HISTORY_VIEW_H

#include <QHBoxLayout>
#include <QList>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

#include "Sync.h"
#include "TimelineItem.h"

#include "Image.h"
#include "Notice.h"
#include "Text.h"

namespace msgs = matrix::events::messages;
namespace events = matrix::events;

// Contains info about a message shown in the history view
// but not yet confirmed by the homeserver through sync.
struct PendingMessage {
	int txn_id;
	QString body;
	QString event_id;
	TimelineItem *widget;

	PendingMessage(int txn_id, QString body, QString event_id, TimelineItem *widget)
	    : txn_id(txn_id)
	    , body(body)
	    , event_id(event_id)
	    , widget(widget)
	{
	}
};

class TimelineView : public QWidget
{
	Q_OBJECT

public:
	TimelineView(QSharedPointer<MatrixClient> client, QWidget *parent = 0);
	TimelineView(const QJsonArray &events, QSharedPointer<MatrixClient> client, QWidget *parent = 0);
	~TimelineView();

	void addHistoryItem(const events::MessageEvent<msgs::Image> &e, const QString &color, bool with_sender);
	void addHistoryItem(const events::MessageEvent<msgs::Notice> &e, const QString &color, bool with_sender);
	void addHistoryItem(const events::MessageEvent<msgs::Text> &e, const QString &color, bool with_sender);

	int addEvents(const QJsonArray &events);
	void addUserTextMessage(const QString &msg, int txn_id);
	void updatePendingMessage(int txn_id, QString event_id);
	void clear();

public slots:
	void sliderRangeChanged(int min, int max);

private:
	void init();
	void removePendingMessage(const events::MessageEvent<msgs::Text> &e);
	bool isPendingMessage(const events::MessageEvent<msgs::Text> &e, const QString &userid);

	QVBoxLayout *top_layout_;
	QVBoxLayout *scroll_layout_;

	QScrollArea *scroll_area_;
	QWidget *scroll_widget_;

	QString last_sender_;

	QList<PendingMessage> pending_msgs_;
	QSharedPointer<MatrixClient> client_;
};

#endif  // HISTORY_VIEW_H
