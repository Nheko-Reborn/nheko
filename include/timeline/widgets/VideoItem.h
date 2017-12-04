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

#include <QEvent>
#include <QLabel>
#include <QSharedPointer>
#include <QWidget>

#include "MatrixClient.h"

#include <mtx.hpp>

class VideoItem : public QWidget
{
        Q_OBJECT

public:
        VideoItem(QSharedPointer<MatrixClient> client,
                  const mtx::events::RoomEvent<mtx::events::msg::Video> &event,
                  QWidget *parent = nullptr);

        VideoItem(QSharedPointer<MatrixClient> client,
                  const QString &url,
                  const QString &filename,
                  QWidget *parent = nullptr);

private:
        void init();
        QString calculateFileSize(int nbytes) const;

        QUrl url_;
        QString text_;
        QString readableFileSize_;

        QLabel *label_;

        mtx::events::RoomEvent<mtx::events::msg::Video> event_;
        QSharedPointer<MatrixClient> client_;
};
