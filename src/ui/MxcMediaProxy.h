// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractVideoSurface>
#include <QBuffer>
#include <QMediaContent>
#include <QMediaPlayer>
#include <QObject>
#include <QPointer>
#include <QString>

#include "Logging.h"

class TimelineModel;

// I failed to get my own buffer into the MediaPlayer in qml, so just make our own. For that we just
// need the videoSurface property, so that part is really easy!
class MxcMediaProxy : public QMediaPlayer
{
        Q_OBJECT
        Q_PROPERTY(TimelineModel *roomm READ room WRITE setRoom NOTIFY roomChanged REQUIRED)
        Q_PROPERTY(QString eventId READ eventId WRITE setEventId NOTIFY eventIdChanged)
        Q_PROPERTY(QAbstractVideoSurface *videoSurface READ getVideoSurface WRITE setVideoSurface)
        Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged)
public:
        MxcMediaProxy(QObject *parent = nullptr)
          : QMediaPlayer(parent)
        {
                connect(this, &MxcMediaProxy::eventIdChanged, &MxcMediaProxy::startDownload);
                connect(this, &MxcMediaProxy::roomChanged, &MxcMediaProxy::startDownload);
                connect(this,
                        qOverload<QMediaPlayer::Error>(&MxcMediaProxy::error),
                        [this](QMediaPlayer::Error error) {
                                nhlog::ui()->info("Media player error {} and errorStr {}",
                                                  error,
                                                  this->errorString().toStdString());
                        });
                connect(this,
                        &MxcMediaProxy::mediaStatusChanged,
                        [this](QMediaPlayer::MediaStatus status) {
                                nhlog::ui()->info(
                                  "Media player status {} and error {}", status, this->error());
                        });
        }

        bool loaded() const { return buffer.size() > 0; }
        QString eventId() const { return eventId_; }
        TimelineModel *room() const { return room_; }
        void setEventId(QString newEventId)
        {
                eventId_ = newEventId;
                emit eventIdChanged();
        }
        void setRoom(TimelineModel *room)
        {
                room_ = room;
                emit roomChanged();
        }
        void setVideoSurface(QAbstractVideoSurface *surface);
        QAbstractVideoSurface *getVideoSurface();

signals:
        void roomChanged();
        void eventIdChanged();
        void loadedChanged();
        void newBuffer(QMediaContent, QIODevice *buf);

private slots:
        void startDownload();

private:
        TimelineModel *room_ = nullptr;
        QString eventId_;
        QString filename_;
        QBuffer buffer;
        QAbstractVideoSurface *m_surface = nullptr;
};
