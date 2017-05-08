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

#include "ImageItem.h"
#include "TimelineItem.h"
#include "TimelineViewManager.h"

namespace events = matrix::events;
namespace msgs = matrix::events::messages;

TimelineItem::TimelineItem(const QString &userid, const QString &color, const QString &body, QWidget *parent)
    : QWidget(parent)
{
	generateTimestamp(QDateTime::currentDateTime());
	generateBody(TimelineViewManager::displayName(userid), color, body);
	setupLayout();
}

TimelineItem::TimelineItem(const QString &body, QWidget *parent)
    : QWidget(parent)
{
	generateTimestamp(QDateTime::currentDateTime());
	generateBody(body);
	setupLayout();
}

TimelineItem::TimelineItem(ImageItem *image, const events::MessageEvent<msgs::Image> &event, const QString &color, QWidget *parent)
    : QWidget(parent)
{
	auto timestamp = QDateTime::fromMSecsSinceEpoch(event.timestamp());
	generateTimestamp(timestamp);
	generateBody(TimelineViewManager::displayName(event.sender()), color, "");

	top_layout_ = new QHBoxLayout();
	top_layout_->setMargin(0);
	top_layout_->addWidget(time_label_);

	auto right_layout = new QVBoxLayout();
	right_layout->addWidget(content_label_);
	right_layout->addWidget(image);

	top_layout_->addLayout(right_layout);
	top_layout_->addStretch(1);

	setLayout(top_layout_);
}

TimelineItem::TimelineItem(ImageItem *image, const events::MessageEvent<msgs::Image> &event, QWidget *parent)
    : QWidget(parent)
{
	auto timestamp = QDateTime::fromMSecsSinceEpoch(event.timestamp());
	generateTimestamp(timestamp);

	top_layout_ = new QHBoxLayout();
	top_layout_->setMargin(0);
	top_layout_->addWidget(time_label_);
	top_layout_->addWidget(image, 1);
	top_layout_->addStretch(1);

	setLayout(top_layout_);
}

TimelineItem::TimelineItem(const events::MessageEvent<msgs::Notice> &event, bool with_sender, const QString &color, QWidget *parent)
    : QWidget(parent)
{
	auto body = event.content().body().trimmed().toHtmlEscaped();
	auto timestamp = QDateTime::fromMSecsSinceEpoch(event.timestamp());

	generateTimestamp(timestamp);

	body = "<i style=\"color: #565E5E\">" + body + "</i>";

	if (with_sender)
		generateBody(TimelineViewManager::displayName(event.sender()), color, body);
	else
		generateBody(body);

	setupLayout();
}

TimelineItem::TimelineItem(const events::MessageEvent<msgs::Text> &event, bool with_sender, const QString &color, QWidget *parent)
    : QWidget(parent)
{
	auto body = event.content().body().trimmed().toHtmlEscaped();
	auto timestamp = QDateTime::fromMSecsSinceEpoch(event.timestamp());

	generateTimestamp(timestamp);

	if (with_sender)
		generateBody(TimelineViewManager::displayName(event.sender()), color, body);
	else
		generateBody(body);

	setupLayout();
}

void TimelineItem::generateBody(const QString &body)
{
	content_label_ = new QLabel(this);
	content_label_->setWordWrap(true);
	content_label_->setAlignment(Qt::AlignTop);
	content_label_->setStyleSheet("margin: 0;");
	QString content(
		"<html>"
		"<head/>"
		"<body>"
		"   <span style=\"font-size: 10pt; color: #171919;\">"
		"   %1"
		"   </span>"
		"</body>"
		"</html>");
	content_label_->setText(content.arg(replaceEmoji(body)));
	content_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
}

void TimelineItem::generateBody(const QString &userid, const QString &color, const QString &body)
{
	auto sender = userid;

	// TODO: Fix this by using a UserId type.
	if (userid.split(":")[0].split("@").size() > 1)
		sender = userid.split(":")[0].split("@")[1];

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
		"   <span style=\"font-size: 10pt; color: #171919;\">"
		"   %3"
		"   </span>"
		"</body>"
		"</html>");
	content_label_->setText(content.arg(color).arg(sender).arg(replaceEmoji(body)));
	content_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
}

void TimelineItem::generateTimestamp(const QDateTime &time)
{
	auto local_time = time.toString("HH:mm");

	time_label_ = new QLabel(this);
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
}

void TimelineItem::setupLayout()
{
	if (time_label_ == nullptr) {
		qWarning() << "TimelineItem: Time label is not initialized";
		return;
	}

	if (content_label_ == nullptr) {
		qWarning() << "TimelineItem: Content label is not initialized";
		return;
	}

	top_layout_ = new QHBoxLayout();
	top_layout_->setMargin(0);
	top_layout_->addWidget(time_label_);
	top_layout_->addWidget(content_label_, 1);

	setLayout(top_layout_);
}

QString TimelineItem::replaceEmoji(const QString &body)
{
	QString fmtBody = "";

	for (auto &c : body) {
		int code = c.unicode();

		// TODO: Be more precise here.
		if (code > 9000)
			fmtBody += "<span style=\"font-family: Emoji One; font-size: 16px\">" + QString(c) + "</span>";
		else
			fmtBody += c;
	}

	return fmtBody;
}

TimelineItem::~TimelineItem()
{
}
