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
#include <QFile>
#include <QFileDialog>
#include <QImageReader>
#include <QPainter>
#include <QStyleOption>

#include "Config.h"
#include "TextInputWidget.h"

FilteredTextEdit::FilteredTextEdit(QWidget *parent)
  : QTextEdit(parent)
{
        setAcceptRichText(false);

        typingTimer_ = new QTimer(this);
        typingTimer_->setInterval(1000);
        typingTimer_->setSingleShot(true);

        connect(typingTimer_, &QTimer::timeout, this, &FilteredTextEdit::stopTyping);
}

void
FilteredTextEdit::keyPressEvent(QKeyEvent *event)
{
        if (!typingTimer_->isActive()) {
                emit startedTyping();
        }

        typingTimer_->start();

        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
                stopTyping();

                emit enterPressed();
        } else {
                QTextEdit::keyPressEvent(event);
        }
}

void
FilteredTextEdit::stopTyping()
{
        typingTimer_->stop();
        emit stoppedTyping();
}

TextInputWidget::TextInputWidget(QWidget *parent)
  : QFrame(parent)
{
        setFont(QFont("Emoji One"));

        setFixedHeight(conf::textInput::height);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setCursor(Qt::ArrowCursor);
        setStyleSheet("background-color: #fff;");

        topLayout_ = new QHBoxLayout();
        topLayout_->setSpacing(0);
        topLayout_->setContentsMargins(15, 0, 15, 5);

        QIcon send_file_icon;
        send_file_icon.addFile(":/icons/icons/ui/paper-clip-outline.png");

        sendFileBtn_ = new FlatButton(this);
        sendFileBtn_->setIcon(send_file_icon);
        sendFileBtn_->setIconSize(QSize(24, 24));

        spinner_ = new LoadingIndicator(this);
        spinner_->setFixedHeight(32);
        spinner_->setFixedWidth(32);
        spinner_->hide();

        QFont font;
        font.setPixelSize(conf::textInputFontSize);

        input_ = new FilteredTextEdit(this);
        input_->setFixedHeight(32);
        input_->setFont(font);
        input_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        input_->setPlaceholderText(tr("Write a message..."));
        input_->setStyleSheet("color: #333333; border: none; padding-top: 5px; margin: 0 5px");

        sendMessageBtn_ = new FlatButton(this);

        QIcon send_message_icon;
        send_message_icon.addFile(":/icons/icons/ui/cursor.png");
        sendMessageBtn_->setIcon(send_message_icon);
        sendMessageBtn_->setIconSize(QSize(24, 24));

        emojiBtn_ = new EmojiPickButton(this);

        QIcon emoji_icon;
        emoji_icon.addFile(":/icons/icons/ui/smile.png");
        emojiBtn_->setIcon(emoji_icon);
        emojiBtn_->setIconSize(QSize(24, 24));

        topLayout_->addWidget(sendFileBtn_);
        topLayout_->addWidget(input_);
        topLayout_->addWidget(emojiBtn_);
        topLayout_->addWidget(sendMessageBtn_);

        setLayout(topLayout_);

        connect(sendMessageBtn_, SIGNAL(clicked()), this, SLOT(onSendButtonClicked()));
        connect(sendFileBtn_, SIGNAL(clicked()), this, SLOT(openFileSelection()));
        connect(input_, SIGNAL(enterPressed()), sendMessageBtn_, SIGNAL(clicked()));
        connect(emojiBtn_,
                SIGNAL(emojiSelected(const QString &)),
                this,
                SLOT(addSelectedEmoji(const QString &)));

        connect(input_, &FilteredTextEdit::startedTyping, this, &TextInputWidget::startedTyping);

        connect(input_, &FilteredTextEdit::stoppedTyping, this, &TextInputWidget::stoppedTyping);
}

void
TextInputWidget::addSelectedEmoji(const QString &emoji)
{
        QTextCursor cursor = input_->textCursor();

        QFont emoji_font("Emoji One");
        emoji_font.setPixelSize(conf::emojiSize);

        QFont text_font("Open Sans");
        text_font.setPixelSize(conf::fontSize);

        QTextCharFormat charfmt;
        charfmt.setFont(emoji_font);
        input_->setCurrentCharFormat(charfmt);

        input_->insertPlainText(emoji);
        cursor.movePosition(QTextCursor::End);

        charfmt.setFont(text_font);
        input_->setCurrentCharFormat(charfmt);

        input_->show();
}

void
TextInputWidget::onSendButtonClicked()
{
        auto msgText = input_->document()->toPlainText().trimmed();

        if (msgText.isEmpty())
                return;

        if (msgText.startsWith(EMOTE_COMMAND)) {
                auto text = parseEmoteCommand(msgText);

                if (!text.isEmpty())
                        emit sendEmoteMessage(text);
        } else if (msgText.startsWith(JOIN_COMMAND)) {
                auto room = parseJoinCommand(msgText);

                if (!room.isEmpty())
                        emit sendJoinRoomRequest(room);
        } else {
                emit sendTextMessage(msgText);
        }

        input_->clear();
}

QString
TextInputWidget::parseJoinCommand(const QString &cmd)
{
        auto room = cmd.right(cmd.size() - JOIN_COMMAND.size()).trimmed();

        if (!room.isEmpty())
                return room;

        return QString("");
}

QString
TextInputWidget::parseEmoteCommand(const QString &cmd)
{
        auto text = cmd.right(cmd.size() - EMOTE_COMMAND.size()).trimmed();

        if (!text.isEmpty())
                return text;

        return QString("");
}

void
TextInputWidget::openFileSelection()
{
        QStringList supportedFiles;
        supportedFiles << "jpeg"
                       << "gif"
                       << "png"
                       << "bmp"
                       << "tiff"
                       << "webp";

        auto fileName = QFileDialog::getOpenFileName(
          this,
          tr("Select an image"),
          "",
          tr("Image Files (*.bmp *.gif *.jpg *.jpeg *.png *.tiff *.webp)"));

        if (fileName.isEmpty())
                return;

        auto imageFormat = QString(QImageReader::imageFormat(fileName));
        if (!supportedFiles.contains(imageFormat)) {
                qDebug() << "Unsupported image format for" << fileName;
                return;
        }

        emit uploadImage(fileName);
        showUploadSpinner();
}

void
TextInputWidget::showUploadSpinner()
{
        topLayout_->removeWidget(sendFileBtn_);
        sendFileBtn_->hide();

        topLayout_->insertWidget(0, spinner_);
        spinner_->start();
}

void
TextInputWidget::hideUploadSpinner()
{
        topLayout_->removeWidget(spinner_);
        topLayout_->insertWidget(0, sendFileBtn_);
        sendFileBtn_->show();
        spinner_->stop();
}

TextInputWidget::~TextInputWidget() {}

void
TextInputWidget::stopTyping()
{
        input_->stopTyping();
}

void
TextInputWidget::focusInEvent(QFocusEvent *event)
{
        input_->setFocus(event->reason());
}
