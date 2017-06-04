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
#include <QFontDatabase>
#include <QRegExp>

#include "AvatarProvider.h"
#include "ImageItem.h"
#include "TimelineItem.h"
#include "TimelineViewManager.h"

static const QRegExp URL_REGEX("((?:https?|ftp)://\\S+)");
static const QString URL_HTML = "<a href=\"\\1\" style=\"color: #333333\">\\1</a>";

namespace events = matrix::events;
namespace msgs = matrix::events::messages;

void TimelineItem::init()
{
	userAvatar_ = nullptr;
	timestamp_ = nullptr;
	userName_ = nullptr;
	body_ = nullptr;

	QFontDatabase db;

	bodyFont_ = db.font("Open Sans", "Regular", 10);
	usernameFont_ = db.font("Open Sans", "Bold", 10);
	timestampFont_ = db.font("Open Sans", "Regular", 7);

	topLayout_ = new QHBoxLayout(this);
	sideLayout_ = new QVBoxLayout();
	mainLayout_ = new QVBoxLayout();
	headerLayout_ = new QHBoxLayout();

	topLayout_->setContentsMargins(7, 0, 0, 0);
	topLayout_->setSpacing(9);

	topLayout_->addLayout(sideLayout_);
	topLayout_->addLayout(mainLayout_, 1);
}

TimelineItem::TimelineItem(const QString &userid, const QString &color, QString body, QWidget *parent)
    : QWidget(parent)
{
	init();

	body.replace(URL_REGEX, URL_HTML);
	auto displayName = TimelineViewManager::displayName(userid);

	generateTimestamp(QDateTime::currentDateTime());
	generateBody(displayName, color, body);

	setupAvatarLayout(displayName);

	mainLayout_->addLayout(headerLayout_);
	mainLayout_->addWidget(body_);
	mainLayout_->setMargin(0);
	mainLayout_->setSpacing(0);

	AvatarProvider::resolve(userid, this);
}

TimelineItem::TimelineItem(QString body, QWidget *parent)
    : QWidget(parent)
{
	init();

	body.replace(URL_REGEX, URL_HTML);

	generateTimestamp(QDateTime::currentDateTime());
	generateBody(body);

	setupSimpleLayout();

	mainLayout_->addWidget(body_);
	mainLayout_->setMargin(0);
	mainLayout_->setSpacing(2);
}

TimelineItem::TimelineItem(ImageItem *image, const events::MessageEvent<msgs::Image> &event, const QString &color, QWidget *parent)
    : QWidget(parent)
{
	init();

	auto timestamp = QDateTime::fromMSecsSinceEpoch(event.timestamp());
	auto displayName = TimelineViewManager::displayName(event.sender());

	generateTimestamp(timestamp);
	generateBody(displayName, color, "");

	setupAvatarLayout(displayName);

	auto imageLayout = new QHBoxLayout();
	imageLayout->addWidget(image);
	imageLayout->addStretch(1);

	mainLayout_->addLayout(headerLayout_);
	mainLayout_->addLayout(imageLayout);
	mainLayout_->setContentsMargins(0, 4, 0, 0);
	mainLayout_->setSpacing(0);

	AvatarProvider::resolve(event.sender(), this);
}

TimelineItem::TimelineItem(ImageItem *image, const events::MessageEvent<msgs::Image> &event, QWidget *parent)
    : QWidget(parent)
{
	init();

	auto timestamp = QDateTime::fromMSecsSinceEpoch(event.timestamp());
	generateTimestamp(timestamp);

	setupSimpleLayout();

	auto imageLayout = new QHBoxLayout();
	imageLayout->setMargin(0);
	imageLayout->addWidget(image);
	imageLayout->addStretch(1);

	mainLayout_->addLayout(imageLayout);
	mainLayout_->setContentsMargins(0, 4, 0, 0);
	mainLayout_->setSpacing(2);
}

TimelineItem::TimelineItem(const events::MessageEvent<msgs::Notice> &event, bool with_sender, const QString &color, QWidget *parent)
    : QWidget(parent)
{
	init();

	auto body = event.content().body().trimmed().toHtmlEscaped();
	auto timestamp = QDateTime::fromMSecsSinceEpoch(event.timestamp());

	generateTimestamp(timestamp);

	body.replace(URL_REGEX, URL_HTML);
	body = "<i style=\"color: #565E5E\">" + body + "</i>";

	if (with_sender) {
		auto displayName = TimelineViewManager::displayName(event.sender());

		generateBody(displayName, color, body);
		setupAvatarLayout(displayName);

		mainLayout_->addLayout(headerLayout_);
		mainLayout_->addWidget(body_);
		mainLayout_->setMargin(0);
		mainLayout_->setSpacing(0);

		AvatarProvider::resolve(event.sender(), this);
	} else {
		generateBody(body);

		setupSimpleLayout();

		mainLayout_->addWidget(body_);
		mainLayout_->setMargin(0);
		mainLayout_->setSpacing(2);
	}
}

TimelineItem::TimelineItem(const events::MessageEvent<msgs::Text> &event, bool with_sender, const QString &color, QWidget *parent)
    : QWidget(parent)
{
	init();

	auto body = event.content().body().trimmed().toHtmlEscaped();
	auto timestamp = QDateTime::fromMSecsSinceEpoch(event.timestamp());

	generateTimestamp(timestamp);

	body.replace(URL_REGEX, URL_HTML);

	if (with_sender) {
		auto displayName = TimelineViewManager::displayName(event.sender());
		generateBody(displayName, color, body);

		setupAvatarLayout(displayName);

		mainLayout_->addLayout(headerLayout_);
		mainLayout_->addWidget(body_);
		mainLayout_->setMargin(0);
		mainLayout_->setSpacing(0);

		AvatarProvider::resolve(event.sender(), this);
	} else {
		generateBody(body);

		setupSimpleLayout();

		mainLayout_->addWidget(body_);
		mainLayout_->setMargin(0);
		mainLayout_->setSpacing(2);
	}
}

// Only the body is displayed.
void TimelineItem::generateBody(const QString &body)
{
	QString content("<span style=\"color: #171919;\">%1</span>");

	body_ = new QLabel(this);
	body_->setWordWrap(true);
	body_->setFont(bodyFont_);
	body_->setText(content.arg(replaceEmoji(body)));
	body_->setAlignment(Qt::AlignTop);

	body_->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextBrowserInteraction);
	body_->setOpenExternalLinks(true);
}

// The username/timestamp is displayed along with the message body.
void TimelineItem::generateBody(const QString &userid, const QString &color, const QString &body)
{
	auto sender = userid;

	// TODO: Fix this by using a UserId type.
	if (userid.split(":")[0].split("@").size() > 1)
		sender = userid.split(":")[0].split("@")[1];

	QString userContent("<span style=\"color: %1\"> %2 </span>");
	QString bodyContent("<span style=\"color: #171717;\"> %1 </span>");

	userName_ = new QLabel(this);
	userName_->setFont(usernameFont_);
	userName_->setText(userContent.arg(color).arg(sender));
	userName_->setAlignment(Qt::AlignTop);

	if (body.isEmpty())
		return;

	body_ = new QLabel(this);
	body_->setFont(bodyFont_);
	body_->setWordWrap(true);
	body_->setAlignment(Qt::AlignTop);
	body_->setText(bodyContent.arg(replaceEmoji(body)));
	body_->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextBrowserInteraction);
	body_->setOpenExternalLinks(true);
}

void TimelineItem::generateTimestamp(const QDateTime &time)
{
	QString msg("<span style=\"color: #5d6565;\"> %1 </span>");

	timestamp_ = new QLabel(this);
	timestamp_->setFont(timestampFont_);
	timestamp_->setText(msg.arg(time.toString("HH:mm")));
	timestamp_->setAlignment(Qt::AlignTop);
	timestamp_->setStyleSheet("margin-top: 2px;");
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

void TimelineItem::setupAvatarLayout(const QString &userName)
{
	topLayout_->setContentsMargins(7, 6, 0, 0);

	userAvatar_ = new Avatar(this);
	userAvatar_->setLetter(QChar(userName[0]).toUpper());
	userAvatar_->setBackgroundColor(QColor("#eee"));
	userAvatar_->setTextColor(QColor("black"));
	userAvatar_->setSize(32);

	// TODO: The provided user name should be a UserId class
	if (userName[0] == '@' && userName.size() > 1)
		userAvatar_->setLetter(QChar(userName[1]).toUpper());

	sideLayout_->addWidget(userAvatar_);
	sideLayout_->addStretch(1);
	sideLayout_->setMargin(0);
	sideLayout_->setSpacing(0);

	headerLayout_->addWidget(userName_);
	headerLayout_->addWidget(timestamp_, 1);
	headerLayout_->setMargin(0);
}

void TimelineItem::setupSimpleLayout()
{
	sideLayout_->addWidget(timestamp_);
	sideLayout_->addStretch(1);

	topLayout_->setContentsMargins(9, 0, 0, 0);
}

void TimelineItem::setUserAvatar(const QImage &avatar)
{
	if (userAvatar_ == nullptr)
		return;

	userAvatar_->setImage(avatar);
}

TimelineItem::~TimelineItem()
{
}
