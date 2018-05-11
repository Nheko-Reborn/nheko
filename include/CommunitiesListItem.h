#pragma once

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QSharedPointer>
#include <QWidget>

#include "Community.h"
#include "Config.h"
#include "ui/Theme.h"

class RippleOverlay;

class CommunitiesListItem : public QWidget
{
        Q_OBJECT
        Q_PROPERTY(QColor highlightedBackgroundColor READ highlightedBackgroundColor WRITE
                     setHighlightedBackgroundColor)
        Q_PROPERTY(
          QColor hoverBackgroundColor READ hoverBackgroundColor WRITE setHoverBackgroundColor)
        Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)

        Q_PROPERTY(QColor avatarFgColor READ avatarFgColor WRITE setAvatarFgColor)
        Q_PROPERTY(QColor avatarBgColor READ avatarBgColor WRITE setAvatarBgColor)

public:
        CommunitiesListItem(QSharedPointer<Community> community,
                            QString community_id,
                            QWidget *parent = nullptr);

        void setCommunity(QSharedPointer<Community> community) { community_ = community; };

        bool isPressed() const { return isPressed_; }
        void setAvatar(const QImage &img);

        QColor highlightedBackgroundColor() const { return highlightedBackgroundColor_; }
        QColor hoverBackgroundColor() const { return hoverBackgroundColor_; }
        QColor backgroundColor() const { return backgroundColor_; }

        QColor avatarFgColor() const { return avatarFgColor_; }
        QColor avatarBgColor() const { return avatarBgColor_; }

        void setHighlightedBackgroundColor(QColor &color) { highlightedBackgroundColor_ = color; }
        void setHoverBackgroundColor(QColor &color) { hoverBackgroundColor_ = color; }
        void setBackgroundColor(QColor &color) { backgroundColor_ = color; }

        void setAvatarFgColor(QColor &color) { avatarFgColor_ = color; }
        void setAvatarBgColor(QColor &color) { avatarBgColor_ = color; }

        QSize sizeHint() const override
        {
                return QSize(IconSize + IconSize / 3, IconSize + IconSize / 3);
        }

signals:
        void clicked(const QString &community_id);

public slots:
        void setPressedState(bool state);

protected:
        void mousePressEvent(QMouseEvent *event) override;
        void paintEvent(QPaintEvent *event) override;

private:
        const int IconSize = 36;

        QSharedPointer<Community> community_;
        QString communityId_;
        QString communityName_;
        QString communityShortDescription;

        QPixmap communityAvatar_;

        QColor highlightedBackgroundColor_;
        QColor hoverBackgroundColor_;
        QColor backgroundColor_;

        QColor avatarFgColor_;
        QColor avatarBgColor_;

        bool isPressed_ = false;

        RippleOverlay *rippleOverlay_;
};
