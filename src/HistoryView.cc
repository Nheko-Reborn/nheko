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
#include <QScrollBar>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>

#include "HistoryView.h"
#include "HistoryViewItem.h"
#include "HistoryViewManager.h"

HistoryView::HistoryView(const QList<Event> &events, QWidget *parent)
    : QWidget(parent)
{
	init();
	addEvents(events);
}

HistoryView::HistoryView(QWidget *parent)
    : QWidget(parent)
{
	init();
}

void HistoryView::clear()
{
	for (const auto msg : scroll_layout_->children())
		msg->deleteLater();
}

void HistoryView::sliderRangeChanged(int min, int max)
{
	Q_UNUSED(min);
	scroll_area_->verticalScrollBar()->setValue(max);
}

void HistoryView::addEvents(const QList<Event> &events)
{
	for (const auto &event : events) {
		if (event.type() == "m.room.message") {
			auto msg_type = event.content().value("msgtype").toString();

			if (msg_type == "m.text" || msg_type == "m.notice") {
				auto with_sender = last_sender_ != event.sender();
				auto color = HistoryViewManager::NICK_COLORS.value(event.sender());

				if (color.isEmpty()) {
					color = HistoryViewManager::chooseRandomColor();
					HistoryViewManager::NICK_COLORS.insert(event.sender(), color);
				}

				addHistoryItem(event, color, with_sender);
				last_sender_ = event.sender();
			}
		}
	}
}

void HistoryView::init()
{
	top_layout_ = new QVBoxLayout(this);
	top_layout_->setSpacing(0);
	top_layout_->setMargin(0);

	scroll_area_ = new QScrollArea(this);
	scroll_area_->setWidgetResizable(true);
	scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	scroll_widget_ = new QWidget();

	scroll_layout_ = new QVBoxLayout();
	scroll_layout_->addStretch(1);
	scroll_layout_->setSpacing(0);

	scroll_widget_->setLayout(scroll_layout_);

	scroll_area_->setWidget(scroll_widget_);

	top_layout_->addWidget(scroll_area_);

	setLayout(top_layout_);

	connect(scroll_area_->verticalScrollBar(),
		SIGNAL(rangeChanged(int, int)),
		this,
		SLOT(sliderRangeChanged(int, int)));
}

void HistoryView::addHistoryItem(const Event &event, const QString &color, bool with_sender)
{
	// TODO: Probably create another function instead of passing the flag.
	HistoryViewItem *item = new HistoryViewItem(event, with_sender, color, scroll_widget_);
	scroll_layout_->addWidget(item);
}

HistoryView::~HistoryView()
{
}
