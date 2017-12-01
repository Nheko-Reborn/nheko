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

#include <QAbstractTextDocumentLayout>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QImageReader>
#include <QMimeDatabase>
#include <QMimeType>
#include <QPainter>
#include <QStyleOption>

#include "Config.h"
#include "TextInputWidget.h"

static constexpr size_t INPUT_HISTORY_SIZE = 127;

FilteredTextEdit::FilteredTextEdit(QWidget *parent)
  : QTextEdit{parent}
  , history_index_{0}
{
        connect(document()->documentLayout(),
                &QAbstractTextDocumentLayout::documentSizeChanged,
                this,
                &FilteredTextEdit::updateGeometry);
        QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        policy.setHeightForWidth(true);
        setSizePolicy(policy);
        working_history_.push_back("");
        connect(this, &QTextEdit::textChanged, this, &FilteredTextEdit::textChanged);
        setAcceptRichText(false);

        typingTimer_ = new QTimer(this);
        typingTimer_->setInterval(1000);
        typingTimer_->setSingleShot(true);

        connect(typingTimer_, &QTimer::timeout, this, &FilteredTextEdit::stopTyping);
}

void
FilteredTextEdit::keyPressEvent(QKeyEvent *event)
{
        const bool isModifier = (event->modifiers() != Qt::NoModifier);

        if (!isModifier) {
                if (!typingTimer_->isActive())
                        emit startedTyping();

                typingTimer_->start();
        }

        switch (event->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
                if (!(event->modifiers() & Qt::ShiftModifier)) {
                        stopTyping();
                        submit();
                } else {
                        QTextEdit::keyPressEvent(event);
                }
                break;
        case Qt::Key_Up: {
                auto initial_cursor = textCursor();
                QTextEdit::keyPressEvent(event);
                if (textCursor() == initial_cursor &&
                    history_index_ + 1 < working_history_.size()) {
                        ++history_index_;
                        setPlainText(working_history_[history_index_]);
                        moveCursor(QTextCursor::End);
                }
                break;
        }
        case Qt::Key_Down: {
                auto initial_cursor = textCursor();
                QTextEdit::keyPressEvent(event);
                if (textCursor() == initial_cursor && history_index_ > 0) {
                        --history_index_;
                        setPlainText(working_history_[history_index_]);
                        moveCursor(QTextCursor::End);
                }
                break;
        }
        default:
                QTextEdit::keyPressEvent(event);
                break;
        }
}

void
FilteredTextEdit::stopTyping()
{
        typingTimer_->stop();
        emit stoppedTyping();
}

QSize
FilteredTextEdit::sizeHint() const
{
        ensurePolished();
        auto margins = viewportMargins();
        margins += document()->documentMargin();
        QSize size = document()->size().toSize();
        size.rwidth() += margins.left() + margins.right();
        size.rheight() += margins.top() + margins.bottom();
        return size;
}

QSize
FilteredTextEdit::minimumSizeHint() const
{
        ensurePolished();
        auto margins = viewportMargins();
        margins += document()->documentMargin();
        margins += contentsMargins();
        QSize size(fontMetrics().averageCharWidth() * 10,
                   fontMetrics().lineSpacing() + margins.top() + margins.bottom());
        return size;
}

void
FilteredTextEdit::submit()
{
        if (toPlainText().trimmed().isEmpty())
                return;

        if (true_history_.size() == INPUT_HISTORY_SIZE)
                true_history_.pop_back();
        true_history_.push_front(toPlainText());
        working_history_ = true_history_;
        working_history_.push_front("");
        history_index_ = 0;

        QString text = toPlainText();
        if (text.startsWith('/')) {
                int command_end = text.indexOf(' ');
                if (command_end == -1)
                        command_end = text.size();
                auto name = text.mid(1, command_end - 1);
                auto args = text.mid(command_end + 1);
                if (name.isEmpty() || name == "/") {
                        message(args);
                } else {
                        command(name, args);
                }
        } else {
                message(std::move(text));
        }

        clear();
}

void
FilteredTextEdit::textChanged()
{
        working_history_[history_index_] = toPlainText();
}

TextInputWidget::TextInputWidget(QWidget *parent)
  : QFrame(parent)
{
        setFont(QFont("Emoji One"));

        setFixedHeight(conf::textInput::height);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setCursor(Qt::ArrowCursor);

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
        spinner_->setObjectName("FileUploadSpinner");
        spinner_->hide();

        QFont font;
        font.setPixelSize(conf::textInputFontSize);

        input_ = new FilteredTextEdit(this);
        input_->setFixedHeight(32);
        input_->setFont(font);
        input_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        input_->setPlaceholderText(tr("Write a message..."));
        input_->setStyleSheet("border: none; padding-top: 5px;");

        sendMessageBtn_ = new FlatButton(this);

        QIcon send_message_icon;
        send_message_icon.addFile(":/icons/icons/ui/cursor.png");
        sendMessageBtn_->setIcon(send_message_icon);
        sendMessageBtn_->setIconSize(QSize(24, 24));

        emojiBtn_ = new emoji::PickButton(this);

        QIcon emoji_icon;
        emoji_icon.addFile(":/icons/icons/ui/smile.png");
        emojiBtn_->setIcon(emoji_icon);
        emojiBtn_->setIconSize(QSize(24, 24));

        topLayout_->addWidget(sendFileBtn_);
        topLayout_->addWidget(input_);
        topLayout_->addWidget(emojiBtn_);
        topLayout_->addWidget(sendMessageBtn_);

        setLayout(topLayout_);

        connect(sendMessageBtn_, &FlatButton::clicked, input_, &FilteredTextEdit::submit);
        connect(sendFileBtn_, SIGNAL(clicked()), this, SLOT(openFileSelection()));
        connect(input_, &FilteredTextEdit::message, this, &TextInputWidget::sendTextMessage);
        connect(input_, &FilteredTextEdit::command, this, &TextInputWidget::command);
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
TextInputWidget::command(QString command, QString args)
{
        if (command == "me") {
                sendEmoteMessage(args);
        } else if (command == "join") {
                sendJoinRoomRequest(args);
        }
}

void
TextInputWidget::openFileSelection()
{
        const auto fileName =
          QFileDialog::getOpenFileName(this, tr("Select a file"), "", tr("All Files (*)"));

        if (fileName.isEmpty())
                return;

        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForFile(fileName, QMimeDatabase::MatchContent);

        const auto format = mime.name().split("/")[0];

        if (format == "image")
                emit uploadImage(fileName);
        else if (format == "audio")
                emit uploadAudio(fileName);
        else
                emit uploadFile(fileName);

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

void
TextInputWidget::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
