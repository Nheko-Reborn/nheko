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

#include <QSettings>

class RoomSettings
{
public:
        RoomSettings(QString room_id)
        {
                path_                   = QString("notifications/%1").arg(room_id);
                isNotificationsEnabled_ = true;

                QSettings settings;

                if (settings.contains(path_))
                        isNotificationsEnabled_ = settings.value(path_).toBool();
                else
                        settings.setValue(path_, isNotificationsEnabled_);
        };

        bool isNotificationsEnabled() { return isNotificationsEnabled_; };

        void toggleNotifications()
        {
                isNotificationsEnabled_ = !isNotificationsEnabled_;

                QSettings settings;
                settings.setValue(path_, isNotificationsEnabled_);
        }

private:
        QString path_;

        bool isNotificationsEnabled_;
};
