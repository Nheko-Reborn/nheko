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

#pragma once

#include <deque>
#include <optional>

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QPaintEvent>
#include <QTextEdit>
#include <QWidget>

#include "WebRTCSession.h"
#include "dialogs/PreviewUploadOverlay.h"
#include "emoji/PickButton.h"
#include "popups/SuggestionsPopup.h"

struct SearchResult;

class CompletionModel;
class FlatButton;
class LoadingIndicator;
class QCompleter;

class FilteredTextEdit : public QTextEdit
{
        Q_OBJECT

public:
        explicit FilteredTextEdit(QWidget *parent = nullptr);

        void stopTyping();

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

        void submit();

signals:
        void heightChanged(int height);
        void startedTyping();
        void stoppedTyping();
        void startedUpload();
        void message(QString msg);
        void command(QString name, QString args);
        void media(QSharedPointer<QIODevice> data, QString mimeClass, const QString &filename);

        //! Trigger the suggestion popup.
        void showSuggestions(const QString &query);
        void resultsRetrieved(const std::vector<SearchResult> &results);
        void selectNextSuggestion();
        void selectPreviousSuggestion();
        void selectHoveredSuggestion();

public slots:
        void showResults(const std::vector<SearchResult> &results);

protected:
        void keyPressEvent(QKeyEvent *event) override;
        bool canInsertFromMimeData(const QMimeData *source) const override;
        void insertFromMimeData(const QMimeData *source) override;
        void focusOutEvent(QFocusEvent *event) override
        {
                suggestionsPopup_.hide();
                QTextEdit::focusOutEvent(event);
        }

private:
        bool emoji_popup_open_ = false;
        CompletionModel *emoji_completion_model_;
        std::deque<QString> true_history_, working_history_;
        int trigger_pos_; // Where emoji completer was triggered
        size_t history_index_;
        QCompleter *completer_;
        QTimer *typingTimer_;

        SuggestionsPopup suggestionsPopup_;

        enum class AnchorType
        {
                Tab   = 0,
                Sigil = 1,
        };

        AnchorType anchorType_ = AnchorType::Sigil;

        int anchorWidth(AnchorType anchor) { return static_cast<int>(anchor); }

        void closeSuggestions() { suggestionsPopup_.hide(); }
        void resetAnchor() { atTriggerPosition_ = -1; }
        bool isAnchorValid() { return atTriggerPosition_ != -1; }
        bool hasAnchor(int pos, AnchorType anchor)
        {
                return pos == atTriggerPosition_ + anchorWidth(anchor);
        }
        QRect completerRect();
        QString query()
        {
                auto cursor = textCursor();
                cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
                return cursor.selectedText();
        }
        QString textAfterPosition(int pos)
        {
                auto tc = textCursor();
                tc.setPosition(pos);
                tc.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                return tc.selectedText();
        }

        dialogs::PreviewUploadOverlay previewDialog_;

        //! Latest position of the '@' character that triggers the username completer.
        int atTriggerPosition_ = -1;

        void insertCompletion(QString completion);
        void textChanged();
        void uploadData(const QByteArray data, const QString &media, const QString &filename);
        void afterCompletion(int);
        void showPreview(const QMimeData *source, const QStringList &formats);
};

class TextInputWidget : public QWidget
{
        Q_OBJECT

        Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor)

public:
        TextInputWidget(QWidget *parent = nullptr);

        void stopTyping();

        QColor borderColor() const { return borderColor_; }
        void setBorderColor(QColor &color) { borderColor_ = color; }
        void disableInput()
        {
                input_->setEnabled(false);
                input_->setPlaceholderText(tr("Connection lost. Nheko is trying to re-connect..."));
        }
        void enableInput()
        {
                input_->setEnabled(true);
                input_->setPlaceholderText(tr("Write a message..."));
        }

public slots:
        void openFileSelection();
        void hideUploadSpinner();
        void focusLineEdit() { input_->setFocus(); }
        void changeCallButtonState(webrtc::State);

private slots:
        void addSelectedEmoji(const QString &emoji);

signals:
        void heightChanged(int height);

        void uploadMedia(const QSharedPointer<QIODevice> data,
                         QString mimeClass,
                         const QString &filename);
        void callButtonPress();

        void sendJoinRoomRequest(const QString &room);
        void sendInviteRoomRequest(const QString &userid, const QString &reason);
        void sendKickRoomRequest(const QString &userid, const QString &reason);
        void sendBanRoomRequest(const QString &userid, const QString &reason);
        void sendUnbanRoomRequest(const QString &userid, const QString &reason);
        void changeRoomNick(const QString &displayname);

        void startedTyping();
        void stoppedTyping();

protected:
        void focusInEvent(QFocusEvent *event) override;
        void paintEvent(QPaintEvent *) override;

private:
        void showUploadSpinner();

        QHBoxLayout *topLayout_;
        FilteredTextEdit *input_;

        LoadingIndicator *spinner_;

        FlatButton *callBtn_;
        FlatButton *sendFileBtn_;
        FlatButton *sendMessageBtn_;
        emoji::PickButton *emojiBtn_;

        QColor borderColor_;
};
