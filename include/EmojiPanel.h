/*
 * nheko Copyright (C) 2017  Konstantinos Sideris <siderisk@auth.gr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QScrollArea>

#include "EmojiProvider.h"

class EmojiCategory;

class EmojiPanel : public QWidget
{
        Q_OBJECT

public:
        EmojiPanel(QWidget *parent = nullptr);

        void fadeOut();
        void fadeIn();

signals:
        void mouseLeft();
        void emojiSelected(const QString &emoji);

protected:
        void leaveEvent(QEvent *event);
        void paintEvent(QPaintEvent *event);

private:
        void showEmojiCategory(const EmojiCategory *category);

        QPropertyAnimation *animation_;
        QGraphicsOpacityEffect *opacity_;

        EmojiProvider emoji_provider_;

        QScrollArea *scrollArea_;

        int shadowMargin_;

        // Panel dimensions.
        int width_;
        int height_;

        int animationDuration_;
        int categoryIconSize_;
};
