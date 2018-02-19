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
#include <QLineEdit>
#include <QPixmap>
#include <QWidget>

#include "FlatButton.h"

class QMimeData;

namespace dialogs {

class PreviewUploadOverlay : public QWidget
{
        Q_OBJECT
public:
        PreviewUploadOverlay(QWidget *parent = nullptr);

        void setPreview(const QByteArray data, const QString &mime);
        void setPreview(const QString &path);

signals:
        void confirmUpload(const QByteArray data, const QString &media, const QString &filename);

private:
        void init();
        void setLabels(const QString &type, const QString &mime, uint64_t upload_size);

        bool isImage_;
        QPixmap image_;

        QByteArray data_;
        QString filePath_;
        QString mediaType_;

        QLabel titleLabel_;
        QLabel infoLabel_;
        QLineEdit fileName_;

        FlatButton upload_;
        FlatButton cancel_;
};
} // dialogs
