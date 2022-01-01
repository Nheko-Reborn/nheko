// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QWidget>

class QMimeData;

namespace dialogs {

class PreviewUploadOverlay : public QWidget
{
    Q_OBJECT
public:
    PreviewUploadOverlay(QWidget *parent = nullptr);

    void setPreview(const QImage &src, const QString &mime);
    void setPreview(const QByteArray data, const QString &mime);
    void setPreview(const QString &path);
    void keyPressEvent(QKeyEvent *event);

signals:
    void confirmUpload(const QByteArray data, const QString &media, const QString &filename);
    void aborted();

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

    QPushButton upload_;
    QPushButton cancel_;
};
} // dialogs
