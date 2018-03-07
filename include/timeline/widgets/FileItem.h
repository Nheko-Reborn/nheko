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
#include <QIcon>
#include <QMouseEvent>
#include <QSharedPointer>
#include <QWidget>

#include <mtx.hpp>

#include "MatrixClient.h"

class FileItem : public QWidget
{
        Q_OBJECT

        Q_PROPERTY(QColor textColor WRITE setTextColor READ textColor)
        Q_PROPERTY(QColor iconColor WRITE setIconColor READ iconColor)
        Q_PROPERTY(QColor backgroundColor WRITE setBackgroundColor READ backgroundColor)

public:
        FileItem(QSharedPointer<MatrixClient> client,
                 const mtx::events::RoomEvent<mtx::events::msg::File> &event,
                 QWidget *parent = nullptr);

        FileItem(QSharedPointer<MatrixClient> client,
                 const QString &url,
                 const QString &filename,
                 uint64_t size,
                 QWidget *parent = nullptr);

        QSize sizeHint() const override;

        void setTextColor(const QColor &color) { textColor_ = color; }
        void setIconColor(const QColor &color) { iconColor_ = color; }
        void setBackgroundColor(const QColor &color) { backgroundColor_ = color; }

        QColor textColor() const { return textColor_; }
        QColor iconColor() const { return iconColor_; }
        QColor backgroundColor() const { return backgroundColor_; }

protected:
        void paintEvent(QPaintEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;

private:
        void openUrl();
        void init();
        void fileDownloaded(const QByteArray &data);

        QUrl url_;
        QString text_;
        QString readableFileSize_;
        QString filenameToSave_;

        mtx::events::RoomEvent<mtx::events::msg::File> event_;
        QSharedPointer<MatrixClient> client_;

        QIcon icon_;

        QColor textColor_       = QColor("white");
        QColor iconColor_       = QColor("#38A3D8");
        QColor backgroundColor_ = QColor("#333");
};
