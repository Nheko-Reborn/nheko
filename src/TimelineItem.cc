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
#include <QTextEdit>

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

	// Initialize layout spacing variables based on the current font.
	QFontMetrics fm(this->font());
	const int baseWidth = fm.width('A');
	MessageMargin = baseWidth * 1.5;

	EmojiSize = this->font().pointSize() * EmojiFontRatio;

	topLayout_ = new QHBoxLayout(this);
	sideLayout_ = new QVBoxLayout();
	mainLayout_ = new QVBoxLayout();
	headerLayout_ = new QHBoxLayout();

	topLayout_->setContentsMargins(MessageMargin, MessageMargin, 0, 0);
	topLayout_->setSpacing(0);

	topLayout_->addLayout(sideLayout_);
	topLayout_->addLayout(mainLayout_, 1);

	sideLayout_->setMargin(0);
	sideLayout_->setSpacing(0);

	mainLayout_->setContentsMargins(baseWidth * 2, 0, 0, 0);
	mainLayout_->setSpacing(0);

	headerLayout_->setMargin(0);
	headerLayout_->setSpacing(baseWidth / 2);
}

/* 
 * For messages created locally. The avatar and the username are displayed. 
 */
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

	AvatarProvider::resolve(userid, this);
}

/*
 * For messages created locally. Only the text is displayed. 
 */
TimelineItem::TimelineItem(QString body, QWidget *parent)
    : QWidget(parent)
{
	init();

	body.replace(URL_REGEX, URL_HTML);

	generateTimestamp(QDateTime::currentDateTime());
	generateBody(body);

	setupSimpleLayout();

	mainLayout_->addWidget(body_);
}

/*
 * Used to display images. The avatar and the username are displayed.
 */
TimelineItem::TimelineItem(ImageItem *image,
			   const events::MessageEvent<msgs::Image> &event,
			   const QString &color,
			   QWidget *parent)
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

	AvatarProvider::resolve(event.sender(), this);
}

/*
 * Used to display images. Only the image is displayed.
 */
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
}

/*
 * Used to display remote notice messages.
 */
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

		AvatarProvider::resolve(event.sender(), this);
	} else {
		generateBody(body);
		setupSimpleLayout();
	}

	mainLayout_->addWidget(body_);
}

/*
 * Used to display remote text messages.
 */
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

		AvatarProvider::resolve(event.sender(), this);
	} else {
		generateBody(body);
		setupSimpleLayout();
	}

	mainLayout_->addWidget(body_);
}

// Only the body is displayed.
void TimelineItem::generateBody(const QString &body)
{
	QString content("<span style=\"color: black;\"> %1 </span>");

	body_ = new QLabel(this);
	body_->setWordWrap(true);
	body_->setText(content.arg(replaceEmoji(body)));
	body_->setMargin(0);

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

	QFont usernameFont;
	usernameFont.setBold(true);

	userName_ = new QLabel(this);
	userName_->setFont(usernameFont);
	userName_->setText(userContent.arg(color).arg(sender));

	if (body.isEmpty())
		return;

	body_ = new QLabel(this);
	body_->setWordWrap(true);
	body_->setText(bodyContent.arg(replaceEmoji(body)));
	body_->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextBrowserInteraction);
	body_->setOpenExternalLinks(true);
	body_->setMargin(0);
}

void TimelineItem::generateTimestamp(const QDateTime &time)
{
	QString msg("<span style=\"color: #5d6565;\"> %1 </span>");

	QFont timestampFont;
	timestampFont.setPointSize(this->font().pointSize() * TimestampFontRatio);

	QFontMetrics fm(timestampFont);
	int topMargin = QFontMetrics(this->font()).height() - fm.height();

	timestamp_ = new QLabel(this);
	timestamp_->setFont(timestampFont);
	timestamp_->setText(msg.arg(time.toString("HH:mm")));
	timestamp_->setContentsMargins(0, topMargin, 0, 0);
}

QString TimelineItem::replaceEmoji(const QString &body)
{
	QString fmtBody = "";

	for (auto &c : body) {
		int code = c.unicode();

		// TODO: Be more precise here.
		if (code > 9000)
			fmtBody += QString("<span style=\"font-family: Emoji One; font-size: %1px\">").arg(EmojiSize) +
				   QString(c) +
				   "</span>";
		else
			fmtBody += c;
	}

	return fmtBody;
}

void TimelineItem::setupAvatarLayout(const QString &userName)
{
	topLayout_->setContentsMargins(MessageMargin, MessageMargin, 0, 0);

	userAvatar_ = new Avatar(this);
	userAvatar_->setLetter(QChar(userName[0]).toUpper());
	userAvatar_->setBackgroundColor(QColor("#eee"));
	userAvatar_->setTextColor(QColor("black"));
	userAvatar_->setSize(AvatarSize);

	// TODO: The provided user name should be a UserId class
	if (userName[0] == '@' && userName.size() > 1)
		userAvatar_->setLetter(QChar(userName[1]).toUpper());

	sideLayout_->addWidget(userAvatar_);
	sideLayout_->addStretch(1);

	headerLayout_->addWidget(userName_);
	headerLayout_->addWidget(timestamp_, 1);
}

void TimelineItem::setupSimpleLayout()
{
	sideLayout_->addWidget(timestamp_);

	// Keep only the time in plain text.
	QTextEdit htmlText(timestamp_->text());
	QString plainText = htmlText.toPlainText();

	// Align the end of the avatar bubble with the end of the timestamp for
	// messages with and without avatar. Otherwise their bodies would not be aligned.
	int timestampWidth = timestamp_->fontMetrics().boundingRect(plainText).width();
	int offset = std::max(0, AvatarSize - timestampWidth) / 2;

	int defaultFontHeight = QFontMetrics(this->font()).height();

	timestamp_->setAlignment(Qt::AlignTop);
	timestamp_->setContentsMargins(offset, defaultFontHeight - timestamp_->fontMetrics().height(), offset, 0);
	topLayout_->setContentsMargins(MessageMargin, MessageMargin / 3, 0, 0);
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
