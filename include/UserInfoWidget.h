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
class LogoutDialog;
class OverlayModal;

class UserInfoWidget : public QWidget
{
        Q_OBJECT

public:
        UserInfoWidget(QWidget *parent = 0);
        ~UserInfoWidget();

        void setAvatar(const QImage &img);
        void setDisplayName(const QString &name);
        void setUserId(const QString &userid);

        void reset();

signals:
        void logout();

protected:
        void resizeEvent(QResizeEvent *event) override;
        void paintEvent(QPaintEvent *event) override;

private slots:
        void closeLogoutDialog(bool isLoggingOut);

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

        QSharedPointer<OverlayModal> logoutModal_;
        QSharedPointer<LogoutDialog> logoutDialog_;

        int logoutButtonSize_;
};
