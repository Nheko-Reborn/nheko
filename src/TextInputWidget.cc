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
#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QDebug>
#include <QFileDialog>
#include <QImageReader>
#include <QMimeData>
#include <QMimeDatabase>
#include <QMimeType>
#include <QPainter>
#include <QStyleOption>

#include "Config.h"
#include "TextInputWidget.h"

static constexpr size_t INPUT_HISTORY_SIZE = 127;
static constexpr int MAX_TEXTINPUT_HEIGHT  = 120;

FilteredTextEdit::FilteredTextEdit(QWidget *parent)
  : QTextEdit{parent}
  , history_index_{0}
  , previewDialog_{parent}
{
        setFrameStyle(QFrame::NoFrame);
        connect(document()->documentLayout(),
                &QAbstractTextDocumentLayout::documentSizeChanged,
                this,
                &FilteredTextEdit::updateGeometry);
        connect(document()->documentLayout(),
                &QAbstractTextDocumentLayout::documentSizeChanged,
                this,
                [=]() { emit heightChanged(document()->size().toSize().height()); });
        working_history_.push_back("");
        connect(this, &QTextEdit::textChanged, this, &FilteredTextEdit::textChanged);
        setAcceptRichText(false);

        typingTimer_ = new QTimer(this);
        typingTimer_->setInterval(1000);
        typingTimer_->setSingleShot(true);

        connect(typingTimer_, &QTimer::timeout, this, &FilteredTextEdit::stopTyping);
        connect(&previewDialog_,
                &dialogs::PreviewImageOverlay::confirmImageUpload,
                this,
                &FilteredTextEdit::receiveImage);

        previewDialog_.hide();
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

                if (textCursor() == initial_cursor && textCursor().atStart() &&
                    history_index_ + 1 < working_history_.size()) {
                        ++history_index_;
                        setPlainText(working_history_[history_index_]);
                        moveCursor(QTextCursor::End);
                } else if (textCursor() == initial_cursor) {
                        // Move to the start of the text if there aren't any lines to move up to.
                        initial_cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor, 1);
                        setTextCursor(initial_cursor);
                }

                break;
        }
        case Qt::Key_Down: {
                auto initial_cursor = textCursor();
                QTextEdit::keyPressEvent(event);

                if (textCursor() == initial_cursor && textCursor().atEnd() && history_index_ > 0) {
                        --history_index_;
                        setPlainText(working_history_[history_index_]);
                        moveCursor(QTextCursor::End);
                } else if (textCursor() == initial_cursor) {
                        // Move to the end of the text if there aren't any lines to move down to.
                        initial_cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor, 1);
                        setTextCursor(initial_cursor);
                }

                break;
        }
        default:
                QTextEdit::keyPressEvent(event);
                break;
        }
}

bool
FilteredTextEdit::canInsertFromMimeData(const QMimeData *source) const
{
        return (source->hasImage() || QTextEdit::canInsertFromMimeData(source));
}

void
FilteredTextEdit::insertFromMimeData(const QMimeData *source)
{
        if (source->hasImage()) {
                const auto formats = source->formats();
                const auto idx     = formats.indexOf(
                  QRegularExpression{"image/.+", QRegularExpression::CaseInsensitiveOption});

                // Note: in the future we may want to look into what the best choice is from the
                // formats list. For now we will default to PNG format.
                QString type = "png";
                if (idx != -1) {
                        type = formats.at(idx).split('/')[1];
                }

                // Encode raw pixel data of image.
                QByteArray data = source->data("image/" + type);
                previewDialog_.setImageAndCreate(data, type);
                previewDialog_.show();
        } else if (source->hasFormat("x-special/gnome-copied-files") &&
                   QImageReader{source->text()}.canRead()) {
                // Special case for X11 users. See "Notes for X11 Users" in source.
                // Source: http://doc.qt.io/qt-5/qclipboard.html
                previewDialog_.setImageAndCreate(source->text());
                previewDialog_.show();
        } else {
                QTextEdit::insertFromMimeData(source);
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

void
FilteredTextEdit::receiveImage(const QByteArray img, const QString &img_name)
{
        QSharedPointer<QBuffer> buffer{new QBuffer{this}};
        buffer->setData(img);
        emit image(buffer, img_name);
}

TextInputWidget::TextInputWidget(QWidget *parent)
  : QWidget(parent)
{
        setFont(QFont("Emoji One"));

        setFixedHeight(conf::textInput::height);
        setCursor(Qt::ArrowCursor);

        topLayout_ = new QHBoxLayout();
        topLayout_->setSpacing(0);
        topLayout_->setContentsMargins(15, 0, 15, 0);

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
        input_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        input_->setPlaceholderText(tr("Write a message..."));

        connect(input_, &FilteredTextEdit::heightChanged, this, [=](int height) {
                int textInputHeight = std::min(MAX_TEXTINPUT_HEIGHT, std::max(height, 32));
                int widgetHeight =
                  std::min(MAX_TEXTINPUT_HEIGHT, std::max(height, conf::textInput::height));

                setFixedHeight(widgetHeight);
                input_->setFixedHeight(textInputHeight);
        });

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
        connect(input_, &FilteredTextEdit::image, this, &TextInputWidget::uploadImage);
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
        } else if (command == "shrug") {
                sendTextMessage("¯\\_(ツ)_/¯");
        } else if (command == "fliptable") {
                sendTextMessage("(╯°□°)╯︵ ┻━┻");
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

        QSharedPointer<QFile> file{new QFile{fileName, this}};
        if (format == "image")
                emit uploadImage(file, fileName);
        else if (format == "audio")
                emit uploadAudio(file, fileName);
        else
                emit uploadFile(file, fileName);

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
