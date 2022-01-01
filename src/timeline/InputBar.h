// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <deque>

#include <mtx/common.hpp>
#include <mtx/responses/messages.hpp>

class TimelineModel;
class CombinedImagePackModel;
class QMimeData;
class QDropEvent;

enum class MarkdownOverride
{
    NOT_SPECIFIED, // no override set
    ON,
    OFF,
};

class InputBar : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool uploading READ uploading NOTIFY uploadingChanged)
    Q_PROPERTY(bool containsAtRoom READ containsAtRoom NOTIFY containsAtRoomChanged)
    Q_PROPERTY(QString text READ text NOTIFY textChanged)

public:
    explicit InputBar(TimelineModel *parent)
      : QObject()
      , room(parent)
    {
        typingRefresh_.setInterval(10'000);
        typingRefresh_.setSingleShot(true);
        typingTimeout_.setInterval(5'000);
        typingTimeout_.setSingleShot(true);
        connect(&typingRefresh_, &QTimer::timeout, this, &InputBar::startTyping);
        connect(&typingTimeout_, &QTimer::timeout, this, &InputBar::stopTyping);
    }

public slots:
    [[nodiscard]] QString text() const;
    QString previousText();
    QString nextText();
    void setText(const QString &newText);

    [[nodiscard]] bool containsAtRoom() const { return containsAtRoom_; }

    void send();
    void paste(bool fromMouse);
    void insertMimeData(const QMimeData *data);
    void updateState(int selectionStart, int selectionEnd, int cursorPosition, const QString &text);
    void openFileSelection();
    [[nodiscard]] bool uploading() const { return uploading_; }
    void message(const QString &body,
                 MarkdownOverride useMarkdown = MarkdownOverride::NOT_SPECIFIED,
                 bool rainbowify              = false);
    void reaction(const QString &reactedEvent, const QString &reactionKey);
    void sticker(CombinedImagePackModel *model, int row);

private slots:
    void startTyping();
    void stopTyping();

signals:
    void insertText(QString text);
    void textChanged(QString newText);
    void uploadingChanged(bool value);
    void containsAtRoomChanged();

private:
    void emote(const QString &body, bool rainbowify);
    void notice(const QString &body, bool rainbowify);
    void command(const QString &name, QString args);
    void image(const QString &filename,
               const std::optional<mtx::crypto::EncryptedFile> &file,
               const QString &url,
               const QString &mime,
               uint64_t dsize,
               const QSize &dimensions,
               const QString &blurhash);
    void file(const QString &filename,
              const std::optional<mtx::crypto::EncryptedFile> &encryptedFile,
              const QString &url,
              const QString &mime,
              uint64_t dsize);
    void audio(const QString &filename,
               const std::optional<mtx::crypto::EncryptedFile> &file,
               const QString &url,
               const QString &mime,
               uint64_t dsize);
    void video(const QString &filename,
               const std::optional<mtx::crypto::EncryptedFile> &file,
               const QString &url,
               const QString &mime,
               uint64_t dsize);

    void showPreview(const QMimeData &source, const QString &path, const QStringList &formats);
    void setUploading(bool value)
    {
        if (value != uploading_) {
            uploading_ = value;
            emit uploadingChanged(value);
        }
    }

    void updateAtRoom(const QString &t);

    QTimer typingRefresh_;
    QTimer typingTimeout_;
    TimelineModel *room;
    std::deque<QString> history_;
    std::size_t history_index_ = 0;
    int selectionStart = 0, selectionEnd = 0, cursorPosition = 0;
    bool uploading_      = false;
    bool containsAtRoom_ = false;
};
