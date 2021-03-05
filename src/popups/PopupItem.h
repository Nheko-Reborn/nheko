// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>

#include "../AvatarProvider.h"
#include "../ChatPage.h"

class Avatar;
struct SearchResult;
class QLabel;
class QHBoxLayout;

class PopupItem : public QWidget
{
        Q_OBJECT

        Q_PROPERTY(QColor hoverColor READ hoverColor WRITE setHoverColor)
        Q_PROPERTY(bool hovering READ hovering WRITE setHovering)

public:
        PopupItem(QWidget *parent);

        QString selectedText() const { return QString(); }
        QColor hoverColor() const { return hoverColor_; }
        void setHoverColor(QColor &color) { hoverColor_ = color; }

        bool hovering() const { return hovering_; }
        void setHovering(const bool hover) { hovering_ = hover; };

protected:
        void paintEvent(QPaintEvent *event) override;

signals:
        void clicked(const QString &text);

protected:
        QHBoxLayout *topLayout_;
        Avatar *avatar_;
        QColor hoverColor_;

        //! Set if the item is currently being
        //! hovered during tab completion (cycling).
        bool hovering_;
};

class RoomItem : public PopupItem
{
        Q_OBJECT

public:
        RoomItem(QWidget *parent, const RoomSearchResult &res);
        QString selectedText() const { return roomId_; }
        void updateItem(const RoomSearchResult &res);

protected:
        void mousePressEvent(QMouseEvent *event) override;

private:
        QLabel *roomName_;
        QString roomId_;
        RoomSearchResult info_;
};
