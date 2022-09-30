// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QBuffer>
#include <QMovie>
#include <QObject>
#include <QQuickItem>

#include "timeline/TimelineModel.h"

// This is an AnimatedImage, that can draw encrypted images
class MxcAnimatedImage : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(TimelineModel *roomm READ room WRITE setRoom NOTIFY roomChanged REQUIRED)
    Q_PROPERTY(QString eventId READ eventId WRITE setEventId NOTIFY eventIdChanged)
    Q_PROPERTY(bool animatable READ animatable NOTIFY animatableChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged)
    Q_PROPERTY(bool play READ play WRITE setPlay NOTIFY playChanged)
public:
    MxcAnimatedImage(QQuickItem *parent = nullptr)
      : QQuickItem(parent)
    {
        connect(this, &MxcAnimatedImage::eventIdChanged, &MxcAnimatedImage::startDownload);
        connect(this, &MxcAnimatedImage::roomChanged, &MxcAnimatedImage::startDownload);
        connect(&movie, &QMovie::frameChanged, this, &MxcAnimatedImage::newFrame);
        setFlag(QQuickItem::ItemHasContents);
        // setAcceptHoverEvents(true);
    }

    bool animatable() const { return animatable_; }
    bool loaded() const { return buffer.size() > 0; }
    bool play() const { return play_; }
    QString eventId() const { return eventId_; }
    TimelineModel *room() const { return room_; }
    void setEventId(QString newEventId)
    {
        if (eventId_ != newEventId) {
            eventId_ = newEventId;
            emit eventIdChanged();
        }
    }
    void setRoom(TimelineModel *room)
    {
        if (room_ != room) {
            room_ = room;
            emit roomChanged();
        }
    }
    void setPlay(bool newPlay)
    {
        if (play_ != newPlay) {
            play_ = newPlay;
            movie.setPaused(!play_);
            emit playChanged();
        }
    }

    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    QSGNode *updatePaintNode(QSGNode *oldNode,
                             QQuickItem::UpdatePaintNodeData *updatePaintNodeData) override;

signals:
    void roomChanged();
    void eventIdChanged();
    void animatableChanged();
    void loadedChanged();
    void playChanged();

private slots:
    void startDownload();
    void newFrame(int frame)
    {
        currentFrame = frame;
        imageDirty   = true;
        update();
    }

private:
    TimelineModel *room_ = nullptr;
    QString eventId_;
    QString filename_;
    bool animatable_ = false;
    QBuffer buffer;
    QMovie movie;
    int currentFrame = 0;
    bool imageDirty  = true;
    bool play_       = true;
};
