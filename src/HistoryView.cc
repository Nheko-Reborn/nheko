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

#include <random>

#include <QDebug>
#include <QScrollBar>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>

#include "HistoryView.h"
#include "HistoryViewItem.h"

const QList<QString> HistoryView::COLORS({"#FFF46E",
					  "#A58BFF",
					  "#50C9BA",
					  "#9EE6CF",
					  "#FFDD67",
					  "#2980B9",
					  "#FC993C",
					  "#2772DB",
					  "#CB8589",
					  "#DDE8B9",
					  "#55A44E",
					  "#A9EEE6",
					  "#53B759",
					  "#9E3997",
					  "#5D89D5",
					  "#BB86B7",
					  "#50a0cf",
					  "#3C989F",
					  "#5A4592",
					  "#235e5b",
					  "#d58247",
					  "#e0a729",
					  "#a2b636",
					  "#4BBE2E"});

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

void HistoryView::sliderRangeChanged(int min, int max)
{
	Q_UNUSED(min);
	scroll_area_->verticalScrollBar()->setValue(max);
}

QString HistoryView::chooseRandomColor()
{
	std::random_device random_device;
	std::mt19937 engine{random_device()};
	std::uniform_int_distribution<int> dist(0, HistoryView::COLORS.size() - 1);

	return HistoryView::COLORS[dist(engine)];
}

void HistoryView::addEvents(const QList<Event> &events)
{
	for (int i = 0; i < events.size(); i++) {
		auto event = events[i];

		if (event.type() == "m.room.message") {
			auto msg_type = event.content().value("msgtype").toString();

			if (msg_type == "m.text" || msg_type == "m.notice") {
				auto with_sender = last_sender_ != event.sender();
				auto color = nick_colors_.value(event.sender());

				if (color.isEmpty()) {
					color = chooseRandomColor();
					nick_colors_.insert(event.sender(), color);
				}

				addHistoryItem(event, color, with_sender);
				last_sender_ = event.sender();
			} else {
				qDebug() << "Ignoring message" << msg_type;
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
