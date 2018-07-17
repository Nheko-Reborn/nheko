#pragma once

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QSharedPointer>
#include <QWidget>

#include <mtx/responses/groups.hpp>

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
        CommunitiesListItem(QString group_id, QWidget *parent = nullptr);

        void setName(QString name) { name_ = name; }
        bool isPressed() const { return isPressed_; }
        void setAvatar(const QImage &img);

        void setRooms(std::vector<QString> room_ids) { room_ids_ = std::move(room_ids); }
        std::vector<QString> rooms() const { return room_ids_; }

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
        void clicked(const QString &group_id);

public slots:
        void setPressedState(bool state);

protected:
        void mousePressEvent(QMouseEvent *event) override;
        void paintEvent(QPaintEvent *event) override;

private:
        const int IconSize = 36;

        QString resolveName() const;

        std::vector<QString> room_ids_;

        QString name_;
        QString groupId_;
        QPixmap avatar_;

        QColor highlightedBackgroundColor_;
        QColor hoverBackgroundColor_;
        QColor backgroundColor_;

        QColor avatarFgColor_;
        QColor avatarBgColor_;

        bool isPressed_ = false;

        RippleOverlay *rippleOverlay_;
};
