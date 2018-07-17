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

#include <QLabel>
#include <QLayout>

class Avatar;
class FlatButton;
class OverlayModal;

class UserInfoWidget : public QWidget
{
        Q_OBJECT

        Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor)

public:
        UserInfoWidget(QWidget *parent = 0);

        void setAvatar(const QImage &img);
        void setDisplayName(const QString &name);
        void setUserId(const QString &userid);

        void reset();

        QColor borderColor() const { return borderColor_; }
        void setBorderColor(QColor &color) { borderColor_ = color; }

signals:
        void logout();

protected:
        void resizeEvent(QResizeEvent *event) override;
        void paintEvent(QPaintEvent *event) override;

private:
        Avatar *userAvatar_;

        QHBoxLayout *topLayout_;
        QHBoxLayout *avatarLayout_;
        QVBoxLayout *textLayout_;
        QHBoxLayout *buttonLayout_;

        FlatButton *logoutButton_;

        QLabel *displayNameLabel_;
        QLabel *userIdLabel_;

        QString display_name_;
        QString user_id_;

        QImage avatar_image_;

        int logoutButtonSize_;

        QColor borderColor_;
};
