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

#include <QDateTime>
#include <QDebug>

#include "HistoryViewItem.h"

HistoryViewItem::HistoryViewItem(const Event &event, bool with_sender, const QString &color, QWidget *parent)
    : QWidget(parent)
{
	QString sender = "";

	if (with_sender)
		sender = event.sender().split(":")[0].split("@")[1];

	auto body = event.content().value("body").toString();

	auto timestamp = QDateTime::fromMSecsSinceEpoch(event.timestamp());
	auto local_time = timestamp.toString("HH:mm");

	time_label_ = new QLabel(this);
	time_label_->setWordWrap(true);
	QString msg(
		"<html>"
		"<head/>"
		"<body>"
		"   <span style=\"font-size: 7pt; color: #5d6565;\">"
		"   %1"
		"   </span>"
		"</body>"
		"</html>");
	time_label_->setText(msg.arg(local_time));
	time_label_->setStyleSheet("margin-left: 7px; margin-right: 7px; margin-top: 0;");
	time_label_->setAlignment(Qt::AlignTop);

	content_label_ = new QLabel(this);
	content_label_->setWordWrap(true);
	content_label_->setAlignment(Qt::AlignTop);
	content_label_->setStyleSheet("margin: 0;");
	QString content(
		"<html>"
		"<head/>"
		"<body>"
		"   <span style=\"font-size: 10pt; font-weight: 600; color: %1\">"
		"   %2"
		"   </span>"
		"   <span style=\"font-size: 10pt;\">"
		"   %3"
		"   </span>"
		"</body>"
		"</html>");
	content_label_->setText(content.arg(color).arg(sender).arg(body));

	top_layout_ = new QHBoxLayout();
	top_layout_->setMargin(0);

	top_layout_->addWidget(time_label_);
	top_layout_->addWidget(content_label_, 1);

	setLayout(top_layout_);
}

HistoryViewItem::~HistoryViewItem()
{
}
