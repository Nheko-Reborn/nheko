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

#include "dialogs/PreviewUploadOverlay.h"

#include "emoji/PickButton.h"

namespace dialogs {
class PreviewUploadOverlay;
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
        void heightChanged(int height);
        void startedTyping();
        void stoppedTyping();
        void startedUpload();
        void message(QString);
        void command(QString name, QString args);
        void image(QSharedPointer<QIODevice> data, const QString &filename);
        void audio(QSharedPointer<QIODevice> data, const QString &filename);
        void video(QSharedPointer<QIODevice> data, const QString &filename);
        void file(QSharedPointer<QIODevice> data, const QString &filename);

protected:
        void keyPressEvent(QKeyEvent *event) override;
        bool canInsertFromMimeData(const QMimeData *source) const override;
        void insertFromMimeData(const QMimeData *source) override;

private:
        std::deque<QString> true_history_, working_history_;
        size_t history_index_;
        QTimer *typingTimer_;

        dialogs::PreviewUploadOverlay previewDialog_;

        void textChanged();
        void uploadData(const QByteArray data, const QString &media, const QString &filename);
        void afterCompletion(int);
        void showPreview(const QMimeData *source, const QStringList &formats);
};

class TextInputWidget : public QWidget
{
        Q_OBJECT

public:
        TextInputWidget(QWidget *parent = 0);

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

        void uploadImage(const QSharedPointer<QIODevice> data, const QString &filename);
        void uploadFile(const QSharedPointer<QIODevice> data, const QString &filename);
        void uploadAudio(const QSharedPointer<QIODevice> data, const QString &filename);
        void uploadVideo(const QSharedPointer<QIODevice> data, const QString &filename);

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
