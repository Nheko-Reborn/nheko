// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QBuffer>
#include <QMediaPlayer>
#include <QObject>
#include <QPointer>
#include <QQuickItem>
#include <QString>
#include <QUrl>
#include <QVideoSink>

class TimelineModel;

// I failed to get my own buffer into the MediaPlayer in qml, so just make our own. For that we just
// need the videoSurface property, so that part is really easy!
class MxcMediaProxy : public QMediaPlayer
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MxcMedia)

    Q_PROPERTY(TimelineModel *roomm READ room WRITE setRoom NOTIFY roomChanged REQUIRED)
    Q_PROPERTY(QString eventId READ eventId WRITE setEventId NOTIFY eventIdChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged)
    Q_PROPERTY(int orientation READ orientation NOTIFY orientationChanged)

public:
    MxcMediaProxy(QObject *parent = nullptr);

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
    int orientation() const;

signals:
    void roomChanged();
    void eventIdChanged();
    void loadedChanged();
    void newBuffer(QUrl, QIODevice *buf);

    void orientationChanged();
    void videoSurfaceChanged();

private slots:
    void startDownload();

private:
    TimelineModel *room_ = nullptr;
    QString eventId_;
    QString filename_;
    QBuffer buffer;
    QObject *m_surface = nullptr;
};
