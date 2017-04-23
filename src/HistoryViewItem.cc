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

HistoryViewItem::HistoryViewItem(const QString &userid, const QString &color, const QString &body, QWidget *parent)
    : QWidget(parent)
{
	generateTimestamp(QDateTime::currentDateTime());
	generateBody(userid, color, body);
	setupLayout();
}

HistoryViewItem::HistoryViewItem(const QString &body, QWidget *parent)
    : QWidget(parent)
{
	generateTimestamp(QDateTime::currentDateTime());
	generateBody(body);
	setupLayout();
}

HistoryViewItem::HistoryViewItem(const Event &event, bool with_sender, const QString &color, QWidget *parent)
    : QWidget(parent)
{
	auto body = event.content().value("body").toString().trimmed().toHtmlEscaped();
	auto timestamp = QDateTime::fromMSecsSinceEpoch(event.timestamp());

	generateTimestamp(timestamp);

	if (event.content().value("msgtype").toString() == "m.notice")
		body = "<i style=\"color: #565E5E\">" + body + "</i>";

	if (with_sender)
		generateBody(event.sender(), color, body);
	else
		generateBody(body);

	setupLayout();
}

void HistoryViewItem::generateBody(const QString &body)
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

void HistoryViewItem::generateBody(const QString &userid, const QString &color, const QString &body)
{
	auto sender = userid.split(":")[0].split("@")[1];

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

void HistoryViewItem::generateTimestamp(const QDateTime &time)
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

void HistoryViewItem::setupLayout()
{
	if (time_label_ == nullptr) {
		qWarning() << "HistoryViewItem: Time label is not initialized";
		return;
	}

	if (content_label_ == nullptr) {
		qWarning() << "HistoryViewItem: Content label is not initialized";
		return;
	}

	top_layout_ = new QHBoxLayout();
	top_layout_->setMargin(0);
	top_layout_->addWidget(time_label_);
	top_layout_->addWidget(content_label_, 1);

	setLayout(top_layout_);
}

QString HistoryViewItem::replaceEmoji(const QString &body)
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

HistoryViewItem::~HistoryViewItem()
{
}
