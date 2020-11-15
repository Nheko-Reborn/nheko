#pragma once

#include <QObject>
#include <deque>

#include <mtx/common.hpp>
#include <mtx/responses/messages.hpp>

class TimelineModel;
class QMimeData;
class QStringList;

class InputBar : public QObject
{
        Q_OBJECT
        Q_PROPERTY(bool uploading READ uploading NOTIFY uploadingChanged)

public:
        InputBar(TimelineModel *parent)
          : QObject()
          , room(parent)
        {}

public slots:
        void send();
        void paste(bool fromMouse);
        void updateState(int selectionStart, int selectionEnd, int cursorPosition, QString text);
        void openFileSelection();
        bool uploading() const { return uploading_; }

signals:
        void insertText(QString text);
        void uploadingChanged(bool value);

private:
        void message(QString body);
        void emote(QString body);
        void command(QString name, QString args);
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

        void showPreview(const QMimeData &source, QString path, const QStringList &formats);
        void setUploading(bool value)
        {
                if (value != uploading_) {
                        uploading_ = value;
                        emit uploadingChanged(value);
                }
        }

        TimelineModel *room;
        QString text;
        std::deque<QString> history_;
        std::size_t history_index_ = 0;
        int selectionStart = 0, selectionEnd = 0, cursorPosition = 0;
        bool uploading_ = false;
};
