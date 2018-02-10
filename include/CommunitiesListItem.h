#pragma once

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QSharedPointer>
#include <QWidget>

#include "Community.h"
#include "Menu.h"
#include "ui/Theme.h"

class CommunitiesListItem : public QWidget
{
        Q_OBJECT
        Q_PROPERTY(QColor highlightedBackgroundColor READ highlightedBackgroundColor WRITE
                     setHighlightedBackgroundColor)
        Q_PROPERTY(
          QColor hoverBackgroundColor READ hoverBackgroundColor WRITE setHoverBackgroundColor)
        Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)

public:
        CommunitiesListItem(QSharedPointer<Community> community,
                            QString community_id,
                            QWidget *parent = nullptr);

        void setCommunity(QSharedPointer<Community> community);

        inline bool isPressed() const;
        inline void setAvatar(const QImage &avatar_image);

        QColor highlightedBackgroundColor() const { return highlightedBackgroundColor_; }
        QColor hoverBackgroundColor() const { return hoverBackgroundColor_; }
        QColor backgroundColor() const { return backgroundColor_; }

        void setHighlightedBackgroundColor(QColor &color) { highlightedBackgroundColor_ = color; }
        void setHoverBackgroundColor(QColor &color) { hoverBackgroundColor_ = color; }
        void setBackgroundColor(QColor &color) { backgroundColor_ = color; }

        QColor highlightedBackgroundColor_;
        QColor hoverBackgroundColor_;
        QColor backgroundColor_;

signals:
        void clicked(const QString &community_id);

public slots:
        void setPressedState(bool state);

protected:
        void mousePressEvent(QMouseEvent *event) override;
        void paintEvent(QPaintEvent *event) override;
        void contextMenuEvent(QContextMenuEvent *event) override;

private:
        const int IconSize = 55;

        QSharedPointer<Community> community_;
        QString communityId_;
        QString communityName_;
        QString communityShortDescription;

        QPixmap communityAvatar_;

        Menu *menu_;
        bool isPressed_ = false;
};

inline bool
CommunitiesListItem::isPressed() const
{
        return isPressed_;
}

inline void
CommunitiesListItem::setAvatar(const QImage &avatar_image)
{
        communityAvatar_ = QPixmap::fromImage(
          avatar_image.scaled(IconSize, IconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        update();
}

class WorldCommunityListItem : public CommunitiesListItem
{
        Q_OBJECT
public:
        WorldCommunityListItem(QWidget *parent = nullptr);

protected:
        void mousePressEvent(QMouseEvent *event) override;
        void paintEvent(QPaintEvent *event) override;

private:
        const int IconSize = 55;
};
