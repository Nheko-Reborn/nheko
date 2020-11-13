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

#include <QAbstractItemView>
#include <QAbstractTextDocumentLayout>
#include <QBuffer>
#include <QClipboard>
#include <QCompleter>
#include <QFileDialog>
#include <QMimeData>
#include <QMimeDatabase>
#include <QMimeType>
#include <QPainter>
#include <QStyleOption>
#include <QtConcurrent>

#include "Cache.h"
#include "ChatPage.h"
#include "CompletionModel.h"
#include "Logging.h"
#include "TextInputWidget.h"
#include "Utils.h"
#include "emoji/EmojiSearchModel.h"
#include "emoji/Provider.h"
#include "ui/FlatButton.h"
#include "ui/LoadingIndicator.h"

#if defined(Q_OS_MAC)
#include "emoji/MacHelper.h"
#endif

static constexpr size_t INPUT_HISTORY_SIZE = 127;
static constexpr int MAX_TEXTINPUT_HEIGHT  = 120;
static constexpr int ButtonHeight          = 22;

FilteredTextEdit::FilteredTextEdit(QWidget *parent)
  : QTextEdit{parent}
  , history_index_{0}
  , suggestionsPopup_{parent}
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
                [this]() { emit heightChanged(document()->size().toSize().height()); });
        working_history_.push_back("");
        connect(this, &QTextEdit::textChanged, this, &FilteredTextEdit::textChanged);
        setAcceptRichText(false);

        completer_ = new QCompleter(this);
        completer_->setWidget(this);
        auto model = new emoji::EmojiSearchModel(this);
        model->sort(0, Qt::AscendingOrder);
        completer_->setModel((emoji_completion_model_ = new CompletionModel(model, this)));
        completer_->setModelSorting(QCompleter::UnsortedModel);
        completer_->popup()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        completer_->popup()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        connect(completer_,
                QOverload<const QModelIndex &>::of(&QCompleter::activated),
                [this](auto &index) {
                        emoji_popup_open_ = false;
                        auto emoji        = index.data(emoji::EmojiModel::Unicode).toString();
                        insertCompletion(emoji);
                });

        typingTimer_ = new QTimer(this);
        typingTimer_->setInterval(1000);
        typingTimer_->setSingleShot(true);

        connect(typingTimer_, &QTimer::timeout, this, &FilteredTextEdit::stopTyping);
        connect(&previewDialog_,
                &dialogs::PreviewUploadOverlay::confirmUpload,
                this,
                &FilteredTextEdit::uploadData);

        connect(this, &FilteredTextEdit::resultsRetrieved, this, &FilteredTextEdit::showResults);
        connect(
          &suggestionsPopup_, &SuggestionsPopup::itemSelected, this, [this](const QString &text) {
                  suggestionsPopup_.hide();

                  auto cursor   = textCursor();
                  const int end = cursor.position();

                  cursor.setPosition(atTriggerPosition_, QTextCursor::MoveAnchor);
                  cursor.setPosition(end, QTextCursor::KeepAnchor);
                  cursor.removeSelectedText();
                  cursor.insertText(text);
          });

        // For cycling through the suggestions by hitting tab.
        connect(this,
                &FilteredTextEdit::selectNextSuggestion,
                &suggestionsPopup_,
                &SuggestionsPopup::selectNextSuggestion);
        connect(this,
                &FilteredTextEdit::selectPreviousSuggestion,
                &suggestionsPopup_,
                &SuggestionsPopup::selectPreviousSuggestion);
        connect(this, &FilteredTextEdit::selectHoveredSuggestion, this, [this]() {
                suggestionsPopup_.selectHoveredSuggestion<UserItem>();
        });

        previewDialog_.hide();
}

void
FilteredTextEdit::insertCompletion(QString completion)
{
        // Paint the current word and replace it with 'completion'
        auto cur_text = textAfterPosition(trigger_pos_);
        auto tc       = textCursor();
        tc.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, cur_text.length());
        tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, cur_text.length());
        tc.insertText(completion);
        setTextCursor(tc);
}

void
FilteredTextEdit::showResults(const std::vector<SearchResult> &results)
{
        QPoint pos;

        if (isAnchorValid()) {
                auto cursor = textCursor();
                cursor.setPosition(atTriggerPosition_);
                pos = viewport()->mapToGlobal(cursorRect(cursor).topLeft());
        } else {
                auto rect = cursorRect();
                pos       = viewport()->mapToGlobal(rect.topLeft());
        }

        suggestionsPopup_.addUsers(results);
        suggestionsPopup_.move(pos.x(), pos.y() - suggestionsPopup_.height() - 10);
        suggestionsPopup_.show();
}

void
FilteredTextEdit::keyPressEvent(QKeyEvent *event)
{
        const bool isModifier = (event->modifiers() != Qt::NoModifier);

#if defined(Q_OS_MAC)
        if (event->modifiers() == (Qt::ControlModifier | Qt::MetaModifier) &&
            event->key() == Qt::Key_Space)
                MacHelper::showEmojiWindow();
#endif

        if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_U)
                QTextEdit::setText("");

        if (!isModifier) {
                if (!typingTimer_->isActive())
                        emit startedTyping();

                typingTimer_->start();
        }

        // calculate the new query
        if (textCursor().position() < atTriggerPosition_ || !isAnchorValid()) {
                resetAnchor();
                closeSuggestions();
        }

        if (suggestionsPopup_.isVisible()) {
                switch (event->key()) {
                case Qt::Key_Down:
                case Qt::Key_Tab:
                        emit selectNextSuggestion();
                        return;
                case Qt::Key_Enter:
                case Qt::Key_Return:
                        emit selectHoveredSuggestion();
                        return;
                case Qt::Key_Escape:
                        closeSuggestions();
                        return;
                case Qt::Key_Up:
                case Qt::Key_Backtab: {
                        emit selectPreviousSuggestion();
                        return;
                }
                default:
                        break;
                }
        }

        if (emoji_popup_open_) {
                auto fake_key = (event->key() == Qt::Key_Backtab) ? Qt::Key_Up : Qt::Key_Down;
                switch (event->key()) {
                case Qt::Key_Backtab:
                case Qt::Key_Tab: {
                        // Simulate up/down arrow press
                        auto ev = new QKeyEvent(QEvent::KeyPress, fake_key, Qt::NoModifier);
                        QCoreApplication::postEvent(completer_->popup(), ev);
                        return;
                }
                default:
                        break;
                }
        }

        switch (event->key()) {
        case Qt::Key_At:
                atTriggerPosition_ = textCursor().position();
                anchorType_        = AnchorType::Sigil;

                QTextEdit::keyPressEvent(event);
                break;
        case Qt::Key_Tab: {
                auto cursor          = textCursor();
                const int initialPos = cursor.position();

                cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
                auto word = cursor.selectedText();

                const int startOfWord = cursor.position();

                // There is a word to complete.
                if (initialPos != startOfWord) {
                        atTriggerPosition_ = startOfWord;
                        anchorType_        = AnchorType::Tab;

                        emit showSuggestions(word);
                } else {
                        QTextEdit::keyPressEvent(event);
                }

                break;
        }
        case Qt::Key_Colon: {
                QTextEdit::keyPressEvent(event);
                trigger_pos_ = textCursor().position() - 1;
                emoji_completion_model_->setFilterRegExp("");
                emoji_popup_open_ = true;
                break;
        }
        case Qt::Key_Return:
        case Qt::Key_Enter:
                if (emoji_popup_open_) {
                        if (!completer_->popup()->currentIndex().isValid()) {
                                // No completion to select, do normal behavior
                                completer_->popup()->hide();
                                emoji_popup_open_ = false;
                        } else {
                                event->ignore();
                                return;
                        }
                }

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

                if (isModifier)
                        return;

                if (emoji_popup_open_ && textAfterPosition(trigger_pos_).length() > 2) {
                        // Update completion
                        emoji_completion_model_->setFilterRegExp(textAfterPosition(trigger_pos_));
                        completer_->complete(completerRect());
                }

                if (emoji_popup_open_ && (completer_->completionCount() < 1 ||
                                          !textAfterPosition(trigger_pos_)
                                             .contains(QRegularExpression(":[^\r\n\t\f\v :]+$")))) {
                        // No completions for this word or another word than the completer was
                        // started with
                        emoji_popup_open_ = false;
                        completer_->popup()->hide();
                }

                if (textCursor().position() == 0) {
                        resetAnchor();
                        closeSuggestions();
                        return;
                }

                // Check if the current word should be autocompleted.
                auto cursor = textCursor();
                cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
                auto word = cursor.selectedText();

                if (hasAnchor(cursor.position(), anchorType_) && isAnchorValid()) {
                        if (word.isEmpty()) {
                                closeSuggestions();
                                return;
                        }

                        emit showSuggestions(word);
                } else {
                        resetAnchor();
                        closeSuggestions();
                }

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
        qInfo() << "Got mime formats: \n" << source->formats();
        const auto formats = source->formats().filter("/");
        const auto image   = formats.filter("image/", Qt::CaseInsensitive);
        const auto audio   = formats.filter("audio/", Qt::CaseInsensitive);
        const auto video   = formats.filter("video/", Qt::CaseInsensitive);

        if (!image.empty() && source->hasImage()) {
                QImage img = qvariant_cast<QImage>(source->imageData());
                previewDialog_.setPreview(img, image.front());
        } else if (!audio.empty()) {
                showPreview(source, audio);
        } else if (!video.empty()) {
                showPreview(source, video);
        } else if (source->hasUrls()) {
                // Generic file path for any platform.
                QString path;
                for (auto &&u : source->urls()) {
                        if (u.isLocalFile()) {
                                path = u.toLocalFile();
                                break;
                        }
                }

                if (!path.isEmpty() && QFileInfo{path}.exists()) {
                        previewDialog_.setPreview(path);
                } else {
                        qWarning()
                          << "Clipboard does not contain any valid file paths:" << source->urls();
                }
        } else if (source->hasFormat("x-special/gnome-copied-files")) {
                // Special case for X11 users. See "Notes for X11 Users" in source.
                // Source: http://doc.qt.io/qt-5/qclipboard.html

                // This MIME type returns a string with multiple lines separated by '\n'. The first
                // line is the command to perform with the clipboard (not useful to us). The
                // following lines are the file URIs.
                //
                // Source: the nautilus source code in file 'src/nautilus-clipboard.c' in function
                // nautilus_clipboard_get_uri_list_from_selection_data()
                // https://github.com/GNOME/nautilus/blob/master/src/nautilus-clipboard.c

                auto data = source->data("x-special/gnome-copied-files").split('\n');
                if (data.size() < 2) {
                        qWarning() << "MIME format is malformed, cannot perform paste.";
                        return;
                }

                QString path;
                for (int i = 1; i < data.size(); ++i) {
                        QUrl url{data[i]};
                        if (url.isLocalFile()) {
                                path = url.toLocalFile();
                                break;
                        }
                }

                if (!path.isEmpty()) {
                        previewDialog_.setPreview(path);
                } else {
                        qWarning() << "Clipboard does not contain any valid file paths:" << data;
                }
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

QRect
FilteredTextEdit::completerRect()
{
        // Move left edge to the beginning of the word
        auto cursor = textCursor();
        auto rect   = cursorRect();
        cursor.movePosition(
          QTextCursor::Left, QTextCursor::MoveAnchor, textAfterPosition(trigger_pos_).length());
        auto cursor_global_x  = viewport()->mapToGlobal(cursorRect(cursor).topLeft()).x();
        auto rect_global_left = viewport()->mapToGlobal(rect.bottomLeft()).x();
        auto dx               = qAbs(rect_global_left - cursor_global_x);
        rect.moveLeft(rect.left() - dx);

        auto item_height = completer_->popup()->sizeHintForRow(0);
        auto max_height  = item_height * completer_->maxVisibleItems();
        auto height      = (completer_->completionCount() > completer_->maxVisibleItems())
                             ? max_height
                             : completer_->completionCount() * item_height;
        rect.setWidth(completer_->popup()->sizeHintForColumn(0));
        rect.moveBottom(-height);
        return rect;
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
FilteredTextEdit::uploadData(const QByteArray data,
                             const QString &mediaType,
                             const QString &filename)
{
        QSharedPointer<QBuffer> buffer{new QBuffer{this}};
        buffer->setData(data);

        emit startedUpload();

        emit media(buffer, mediaType, filename);
}

void
FilteredTextEdit::showPreview(const QMimeData *source, const QStringList &formats)
{
        // Retrieve data as MIME type.
        auto const &mime = formats.first();
        QByteArray data  = source->data(mime);
        previewDialog_.setPreview(data, mime);
}

TextInputWidget::TextInputWidget(QWidget *parent)
  : QWidget(parent)
{
        QFont f;
        f.setPointSizeF(f.pointSizeF());
        const int fontHeight    = QFontMetrics(f).height();
        const int contentHeight = static_cast<int>(fontHeight * 2.5);
        const int InputHeight   = static_cast<int>(fontHeight * 1.5);

        setFixedHeight(contentHeight);
        setCursor(Qt::ArrowCursor);

        topLayout_ = new QHBoxLayout();
        topLayout_->setSpacing(0);
        topLayout_->setContentsMargins(13, 1, 13, 0);

#ifdef GSTREAMER_AVAILABLE
        callBtn_ = new FlatButton(this);
        changeCallButtonState(webrtc::State::DISCONNECTED);
        connect(&WebRTCSession::instance(),
                &WebRTCSession::stateChanged,
                this,
                &TextInputWidget::changeCallButtonState);
#endif

        QIcon send_file_icon;
        send_file_icon.addFile(":/icons/icons/ui/paper-clip-outline.png");

        sendFileBtn_ = new FlatButton(this);
        sendFileBtn_->setToolTip(tr("Send a file"));
        sendFileBtn_->setIcon(send_file_icon);
        sendFileBtn_->setIconSize(QSize(ButtonHeight, ButtonHeight));

        spinner_ = new LoadingIndicator(this);
        spinner_->setFixedHeight(InputHeight);
        spinner_->setFixedWidth(InputHeight);
        spinner_->setObjectName("FileUploadSpinner");
        spinner_->hide();

        input_ = new FilteredTextEdit(this);
        input_->setFixedHeight(InputHeight);
        input_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        input_->setPlaceholderText(tr("Write a message..."));

        connect(input_,
                &FilteredTextEdit::heightChanged,
                this,
                [this, InputHeight, contentHeight](int height) {
                        int widgetHeight =
                          std::min(MAX_TEXTINPUT_HEIGHT, std::max(height, contentHeight));
                        int textInputHeight =
                          std::min(widgetHeight - 1, std::max(height, InputHeight));

                        setFixedHeight(widgetHeight);
                        input_->setFixedHeight(textInputHeight);

                        emit heightChanged(widgetHeight);
                });
        connect(input_, &FilteredTextEdit::showSuggestions, this, [this](const QString &q) {
                if (q.isEmpty())
                        return;

                QtConcurrent::run([this, q = q.toLower().toStdString()]() {
                        try {
                                emit input_->resultsRetrieved(cache::searchUsers(
                                  ChatPage::instance()->currentRoom().toStdString(), q));
                        } catch (const lmdb::error &e) {
                                nhlog::db()->error("Suggestion retrieval failed: {}", e.what());
                        }
                });
        });

        sendMessageBtn_ = new FlatButton(this);
        sendMessageBtn_->setToolTip(tr("Send a message"));

        QIcon send_message_icon;
        send_message_icon.addFile(":/icons/icons/ui/cursor.png");
        sendMessageBtn_->setIcon(send_message_icon);
        sendMessageBtn_->setIconSize(QSize(ButtonHeight, ButtonHeight));

        emojiBtn_ = new emoji::PickButton(this);
        emojiBtn_->setToolTip(tr("Emoji"));

#if defined(Q_OS_MAC)
        // macOS has a native emoji picker.
        emojiBtn_->hide();
#endif

        QIcon emoji_icon;
        emoji_icon.addFile(":/icons/icons/ui/smile.png");
        emojiBtn_->setIcon(emoji_icon);
        emojiBtn_->setIconSize(QSize(ButtonHeight, ButtonHeight));

#ifdef GSTREAMER_AVAILABLE
        topLayout_->addWidget(callBtn_);
#endif
        topLayout_->addWidget(sendFileBtn_);
        topLayout_->addWidget(input_);
        topLayout_->addWidget(emojiBtn_);
        topLayout_->addWidget(sendMessageBtn_);

        setLayout(topLayout_);

#ifdef GSTREAMER_AVAILABLE
        connect(callBtn_, &FlatButton::clicked, this, &TextInputWidget::callButtonPress);
#endif
        connect(sendMessageBtn_, &FlatButton::clicked, input_, &FilteredTextEdit::submit);
        connect(sendFileBtn_, SIGNAL(clicked()), this, SLOT(openFileSelection()));
        connect(input_, &FilteredTextEdit::message, this, &TextInputWidget::sendTextMessage);
        connect(input_, &FilteredTextEdit::command, this, &TextInputWidget::command);
        connect(input_, &FilteredTextEdit::media, this, &TextInputWidget::uploadMedia);
        connect(emojiBtn_,
                SIGNAL(emojiSelected(const QString &)),
                this,
                SLOT(addSelectedEmoji(const QString &)));

        connect(input_, &FilteredTextEdit::startedTyping, this, &TextInputWidget::startedTyping);

        connect(input_, &FilteredTextEdit::stoppedTyping, this, &TextInputWidget::stoppedTyping);

        connect(
          input_, &FilteredTextEdit::startedUpload, this, &TextInputWidget::showUploadSpinner);
}

void
TextInputWidget::addSelectedEmoji(const QString &emoji)
{
        QTextCursor cursor = input_->textCursor();

        QTextCharFormat charfmt;
        input_->setCurrentCharFormat(charfmt);

        input_->insertPlainText(emoji);
        cursor.movePosition(QTextCursor::End);

        input_->setCurrentCharFormat(charfmt);

        input_->show();
}

void
TextInputWidget::command(QString command, QString args)
{
        if (command == "me") {
                emit sendEmoteMessage(args);
        } else if (command == "join") {
                emit sendJoinRoomRequest(args);
        } else if (command == "invite") {
                emit sendInviteRoomRequest(args.section(' ', 0, 0), args.section(' ', 1, -1));
        } else if (command == "kick") {
                emit sendKickRoomRequest(args.section(' ', 0, 0), args.section(' ', 1, -1));
        } else if (command == "ban") {
                emit sendBanRoomRequest(args.section(' ', 0, 0), args.section(' ', 1, -1));
        } else if (command == "unban") {
                emit sendUnbanRoomRequest(args.section(' ', 0, 0), args.section(' ', 1, -1));
        } else if (command == "roomnick") {
                emit changeRoomNick(args);
        } else if (command == "shrug") {
                emit sendTextMessage("¯\\_(ツ)_/¯");
        } else if (command == "fliptable") {
                emit sendTextMessage("(╯°□°)╯︵ ┻━┻");
        } else if (command == "unfliptable") {
                emit sendTextMessage(" ┯━┯╭( º _ º╭)");
        } else if (command == "sovietflip") {
                emit sendTextMessage("ノ┬─┬ノ ︵ ( \\o°o)\\");
        } else if (command == "clear-timeline") {
                emit clearRoomTimeline();
        } else if (command == "rotate-megolm-session") {
                emit rotateMegolmSession();
        }
}

void
TextInputWidget::openFileSelection()
{
        const QString homeFolder = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        const auto fileName =
          QFileDialog::getOpenFileName(this, tr("Select a file"), homeFolder, tr("All Files (*)"));

        if (fileName.isEmpty())
                return;

        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForFile(fileName, QMimeDatabase::MatchContent);

        const auto format = mime.name().split("/")[0];

        QSharedPointer<QFile> file{new QFile{fileName, this}};

        emit uploadMedia(file, format, QFileInfo(fileName).fileName());

        showUploadSpinner();
}

void
TextInputWidget::showUploadSpinner()
{
        topLayout_->removeWidget(sendFileBtn_);
        sendFileBtn_->hide();

        topLayout_->insertWidget(1, spinner_);
        spinner_->start();
}

void
TextInputWidget::hideUploadSpinner()
{
        topLayout_->removeWidget(spinner_);
        topLayout_->insertWidget(1, sendFileBtn_);
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

void
TextInputWidget::changeCallButtonState(webrtc::State state)
{
        QIcon icon;
        if (state == webrtc::State::ICEFAILED || state == webrtc::State::DISCONNECTED) {
                callBtn_->setToolTip(tr("Place a call"));
                icon.addFile(":/icons/icons/ui/place-call.png");
        } else {
                callBtn_->setToolTip(tr("Hang up"));
                icon.addFile(":/icons/icons/ui/end-call.png");
        }
        callBtn_->setIcon(icon);
        callBtn_->setIconSize(QSize(ButtonHeight * 1.1, ButtonHeight * 1.1));
}
