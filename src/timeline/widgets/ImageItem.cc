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

#include <QBrush>
#include <QDebug>
#include <QDesktopServices>
#include <QFileInfo>
#include <QPainter>
#include <QPixmap>

#include "Config.h"
#include "dialogs/ImageOverlay.h"
#include "timeline/widgets/ImageItem.h"

ImageItem::ImageItem(QSharedPointer<MatrixClient> client,
                     const mtx::events::RoomEvent<mtx::events::msg::Image> &event,
                     QWidget *parent)
  : QWidget(parent)
  , event_{event}
  , client_{client}
{
        setMouseTracking(true);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover, true);

        url_  = QString::fromStdString(event.content.url);
        text_ = QString::fromStdString(event.content.body);

        QList<QString> url_parts = url_.toString().split("mxc://");

        if (url_parts.size() != 2) {
                qDebug() << "Invalid format for image" << url_.toString();
                return;
        }

        QString media_params = url_parts[1];
        url_                 = QString("%1/_matrix/media/r0/download/%2")
                 .arg(client_.data()->getHomeServer().toString(), media_params);

        client_.data()->downloadImage(QString::fromStdString(event.event_id), url_);

        connect(client_.data(),
                SIGNAL(imageDownloaded(const QString &, const QPixmap &)),
                this,
                SLOT(imageDownloaded(const QString &, const QPixmap &)));
}

ImageItem::ImageItem(QSharedPointer<MatrixClient> client,
                     const QString &url,
                     const QSharedPointer<QIODevice> data,
                     const QString &filename,
                     QWidget *parent)
  : QWidget(parent)
  , url_{url}
  , text_{filename}
  , client_{client}
{
        setMouseTracking(true);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover, true);

        QList<QString> url_parts = url_.toString().split("mxc://");

        if (url_parts.size() != 2) {
                qDebug() << "Invalid format for image" << url_.toString();
                return;
        }

        QString media_params = url_parts[1];
        url_                 = QString("%1/_matrix/media/r0/download/%2")
                 .arg(client_.data()->getHomeServer().toString(), media_params);

        if (data.isNull()) {
                qWarning() << "No image data to display";
                return;
        }

        if (data->reset()) {
                QPixmap p;
                p.loadFromData(data->readAll());
                setImage(p);
        } else {
                qWarning() << "Failed to seek to beginning of device:" << data->errorString();
                return;
        }
}

void
ImageItem::imageDownloaded(const QString &event_id, const QPixmap &img)
{
        if (event_id != QString::fromStdString(event_.event_id))
                return;

        setImage(img);
}

void
ImageItem::openUrl()
{
        if (url_.toString().isEmpty())
                return;

        if (!QDesktopServices::openUrl(url_))
                qWarning() << "Could not open url" << url_.toString();
}

void
ImageItem::scaleImage()
{
        if (image_.isNull())
                return;

        auto width_ratio  = (double)max_width_ / (double)image_.width();
        auto height_ratio = (double)max_height_ / (double)image_.height();

        auto min_aspect_ratio = std::min(width_ratio, height_ratio);

        if (min_aspect_ratio > 1) {
                width_  = image_.width();
                height_ = image_.height();
        } else {
                width_  = image_.width() * min_aspect_ratio;
                height_ = image_.height() * min_aspect_ratio;
        }

        setFixedSize(width_, height_);
        scaled_image_ =
          image_.scaled(width_, height_, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

QSize
ImageItem::sizeHint() const
{
        if (image_.isNull())
                return QSize(max_width_, bottom_height_);

        return QSize(width_, height_);
}

void
ImageItem::setImage(const QPixmap &image)
{
        image_ = image;
        scaleImage();
        update();
}

void
ImageItem::mousePressEvent(QMouseEvent *event)
{
        if (event->button() != Qt::LeftButton)
                return;

        if (image_.isNull()) {
                openUrl();
                return;
        }

        if (textRegion_.contains(event->pos())) {
                openUrl();
        } else {
                auto image_dialog = new dialogs::ImageOverlay(image_, this);
                image_dialog->show();
        }
}

void
ImageItem::resizeEvent(QResizeEvent *event)
{
        Q_UNUSED(event);

        scaleImage();
}

void
ImageItem::paintEvent(QPaintEvent *event)
{
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QFont font("Open Sans");
        font.setPixelSize(conf::fontSize);

        QFontMetrics metrics(font);
        const int fontHeight = metrics.ascent();

        if (image_.isNull()) {
                QString elidedText = metrics.elidedText(text_, Qt::ElideRight, max_width_ - 10);

                setFixedSize(metrics.width(elidedText), fontHeight);

                painter.setFont(font);
                painter.setPen(QPen(QColor(66, 133, 244)));
                painter.drawText(QPoint(0, fontHeight), elidedText);

                return;
        }

        imageRegion_ = QRectF(0, 0, width_, height_);

        QPainterPath path;
        path.addRoundedRect(imageRegion_, 5, 5);

        painter.setPen(Qt::NoPen);
        painter.fillPath(path, scaled_image_);
        painter.drawPath(path);

        // Bottom text section
        if (underMouse()) {
                const int textBoxHeight = 2 * fontHeight;

                textRegion_ = QRectF(0, height_ - textBoxHeight, width_, textBoxHeight);

                QPainterPath textPath;
                textPath.addRoundedRect(textRegion_, 0, 0);

                painter.fillPath(textPath, QColor(40, 40, 40, 140));

                QString elidedText = metrics.elidedText(text_, Qt::ElideRight, width_ - 10);

                font.setWeight(80);
                painter.setFont(font);
                painter.setPen(QPen(QColor("white")));

                textRegion_.adjust(5, 0, 5, 0);
                painter.drawText(textRegion_, Qt::AlignVCenter, elidedText);
        }
}
