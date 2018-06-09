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
#include <QMouseEvent>
#include <QSharedPointer>
#include <QWidget>

#include <mtx.hpp>

namespace dialogs {
class ImageOverlay;
}

class ImageItem : public QWidget
{
        Q_OBJECT
public:
        ImageItem(const mtx::events::RoomEvent<mtx::events::msg::Image> &event,
                  QWidget *parent = nullptr);

        ImageItem(const QString &url,
                  const QString &filename,
                  uint64_t size,
                  QWidget *parent = nullptr);

        QSize sizeHint() const override;

public slots:
        //! Show a save as dialog for the image.
        void saveAs();
        void setImage(const QPixmap &image);
        void saveImage(const QString &filename, const QByteArray &data);

signals:
        void imageDownloaded(const QPixmap &img);
        void imageSaved(const QString &filename, const QByteArray &data);

protected:
        void paintEvent(QPaintEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;
        void resizeEvent(QResizeEvent *event) override;

        //! Whether the user can interact with the displayed image.
        bool isInteractive_ = true;

private:
        void init();
        void openUrl();
        void downloadMedia(const QUrl &url);

        int max_width_  = 500;
        int max_height_ = 300;

        int width_;
        int height_;

        QPixmap scaled_image_;
        QPixmap image_;

        QUrl url_;
        QString text_;

        int bottom_height_ = 30;

        QRectF textRegion_;
        QRectF imageRegion_;

        mtx::events::RoomEvent<mtx::events::msg::Image> event_;
};

class StickerItem : public ImageItem
{
        Q_OBJECT

public:
        StickerItem(const mtx::events::Sticker &event, QWidget *parent = nullptr)
          : ImageItem{QString::fromStdString(event.content.url),
                      QString::fromStdString(event.content.body),
                      event.content.info.size,
                      parent}
          , event_{event}
        {
                isInteractive_ = false;
                setCursor(Qt::ArrowCursor);
                setMouseTracking(false);
                setAttribute(Qt::WA_Hover, false);
        }

private:
        mtx::events::Sticker event_;
};
