// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSharedPointer>
#include <QWidget>

#include <set>

#include "Config.h"
#include "ui/Theme.h"

class RippleOverlay;
class QMouseEvent;
class QMenu;

class CommunitiesListItem : public QWidget
{
        Q_OBJECT
        Q_PROPERTY(QColor highlightedBackgroundColor READ highlightedBackgroundColor WRITE
                     setHighlightedBackgroundColor)
        Q_PROPERTY(QColor disabledBackgroundColor READ disabledBackgroundColor WRITE
                     setDisabledBackgroundColor)
        Q_PROPERTY(
          QColor hoverBackgroundColor READ hoverBackgroundColor WRITE setHoverBackgroundColor)
        Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)

        Q_PROPERTY(QColor avatarFgColor READ avatarFgColor WRITE setAvatarFgColor)
        Q_PROPERTY(QColor avatarBgColor READ avatarBgColor WRITE setAvatarBgColor)

public:
        CommunitiesListItem(QString group_id, QWidget *parent = nullptr);

        void setName(QString name);
        bool isPressed() const { return isPressed_; }
        bool isDisabled() const { return isDisabled_; }
        void setAvatar(const QImage &img);

        void setRooms(std::set<QString> room_ids) { room_ids_ = std::move(room_ids); }
        void addRoom(const QString &id) { room_ids_.insert(id); }
        void delRoom(const QString &id) { room_ids_.erase(id); }
        std::set<QString> rooms() const { return room_ids_; }

        bool is_tag() const { return groupId_.startsWith("tag:"); }

        QColor highlightedBackgroundColor() const { return highlightedBackgroundColor_; }
        QColor disabledBackgroundColor() const { return disabledBackgroundColor_; }
        QColor hoverBackgroundColor() const { return hoverBackgroundColor_; }
        QColor backgroundColor() const { return backgroundColor_; }

        QColor avatarFgColor() const { return avatarFgColor_; }
        QColor avatarBgColor() const { return avatarBgColor_; }

        void setHighlightedBackgroundColor(QColor &color) { highlightedBackgroundColor_ = color; }
        void setDisabledBackgroundColor(QColor &color) { disabledBackgroundColor_ = color; }
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
        void isDisabledChanged();

public slots:
        void setPressedState(bool state);
        void setDisabled(bool state);

protected:
        void mousePressEvent(QMouseEvent *event) override;
        void paintEvent(QPaintEvent *event) override;
        void contextMenuEvent(QContextMenuEvent *event) override;

private:
        const int IconSize = 36;

        QString resolveName() const;
        void updateTooltip();

        std::set<QString> room_ids_;

        QString name_;
        QString groupId_;
        QPixmap avatar_;

        QColor highlightedBackgroundColor_;
        QColor disabledBackgroundColor_;
        QColor hoverBackgroundColor_;
        QColor backgroundColor_;

        QColor avatarFgColor_;
        QColor avatarBgColor_;

        bool isPressed_  = false;
        bool isDisabled_ = false;

        RippleOverlay *rippleOverlay_;
        QMenu *menu_;
        QAction *hideRoomsWithTagAction_;
};
