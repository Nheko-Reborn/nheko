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

#include <QHBoxLayout>
#include <QPaintEvent>
#include <QTextEdit>
#include <QWidget>

#include "FlatButton.h"
#include "LoadingIndicator.h"

#include "dialogs/PreviewImageOverlay.h"

#include "emoji/PickButton.h"

namespace dialogs {
class PreviewImageOverlay;
}

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
        void startedTyping();
        void stoppedTyping();
        void message(QString);
        void command(QString name, QString args);
        void image(const QSharedPointer<QIODevice> iodev, const QString &img_name);

protected:
        void keyPressEvent(QKeyEvent *event) override;
        bool canInsertFromMimeData(const QMimeData *source) const override;
        void insertFromMimeData(const QMimeData *source) override;

private:
        std::deque<QString> true_history_, working_history_;
        size_t history_index_;
        QTimer *typingTimer_;

        dialogs::PreviewImageOverlay previewDialog_;

        void textChanged();
        void receiveImage(const QByteArray img, const QString &img_name);
        void afterCompletion(int);
};

class TextInputWidget : public QFrame
{
        Q_OBJECT

public:
        TextInputWidget(QWidget *parent = 0);
        ~TextInputWidget();

        void stopTyping();

public slots:
        void openFileSelection();
        void hideUploadSpinner();
        void focusLineEdit() { input_->setFocus(); }

private slots:
        void addSelectedEmoji(const QString &emoji);

signals:
        void sendTextMessage(QString msg);
        void sendEmoteMessage(QString msg);

        void uploadImage(QSharedPointer<QIODevice> data, const QString &filename);
        void uploadFile(QSharedPointer<QIODevice> data, const QString &filename);
        void uploadAudio(QSharedPointer<QIODevice> data, const QString &filename);

        void sendJoinRoomRequest(const QString &room);

        void startedTyping();
        void stoppedTyping();

protected:
        void focusInEvent(QFocusEvent *event) override;
        void paintEvent(QPaintEvent *) override;

private:
        void showUploadSpinner();
        void command(QString name, QString args);

        QHBoxLayout *topLayout_;
        FilteredTextEdit *input_;

        LoadingIndicator *spinner_;

        FlatButton *sendFileBtn_;
        FlatButton *sendMessageBtn_;
        emoji::PickButton *emojiBtn_;
};
