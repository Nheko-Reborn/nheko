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
#include <QFileDialog>
#include <QFileInfo>
#include <QPainter>
#include <QPixmap>
#include <QUuid>

#include "Config.h"
#include "Utils.h"
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

        auto proxy = client_.data()->downloadImage(url_);

        connect(
          proxy, &DownloadMediaProxy::imageDownloaded, this, [this, proxy](const QPixmap &img) {
                  proxy->deleteLater();
                  setImage(img);
          });
}

ImageItem::ImageItem(QSharedPointer<MatrixClient> client,
                     const QString &url,
                     const QString &filename,
                     uint64_t size,
                     QWidget *parent)
  : QWidget(parent)
  , url_{url}
  , text_{filename}
  , client_{client}
{
        Q_UNUSED(size);

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

        auto proxy = client_.data()->downloadImage(url_);

        connect(
          proxy, &DownloadMediaProxy::imageDownloaded, this, [proxy, this](const QPixmap &img) {
                  proxy->deleteLater();
                  setImage(img);
          });
}

void
ImageItem::openUrl()
{
        if (url_.toString().isEmpty())
                return;

        if (!QDesktopServices::openUrl(url_))
                qWarning() << "Could not open url" << url_.toString();
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
        image_        = image;
        scaled_image_ = utils::scaleDown<QPixmap>(max_width_, max_height_, image_);

        width_  = scaled_image_.width();
        height_ = scaled_image_.height();

        setFixedSize(width_, height_);
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
                auto imgDialog = new dialogs::ImageOverlay(image_);
                imgDialog->show();
        }
}

void
ImageItem::resizeEvent(QResizeEvent *event)
{
        if (!image_)
                return QWidget::resizeEvent(event);

        scaled_image_ = utils::scaleDown<QPixmap>(max_width_, max_height_, image_);

        width_  = scaled_image_.width();
        height_ = scaled_image_.height();

        setFixedSize(width_, height_);
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
        const int fontHeight = metrics.height() + metrics.ascent();

        if (image_.isNull()) {
                QString elidedText = metrics.elidedText(text_, Qt::ElideRight, max_width_ - 10);

                setFixedSize(metrics.width(elidedText), fontHeight);

                painter.setFont(font);
                painter.setPen(QPen(QColor(66, 133, 244)));
                painter.drawText(QPoint(0, fontHeight / 2), elidedText);

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
                const int textBoxHeight = fontHeight / 2 + 6;

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

void
ImageItem::saveAs()
{
        auto filename = QFileDialog::getSaveFileName(this, tr("Save image"), text_);

        if (filename.isEmpty())
                return;

        auto proxy = client_->downloadFile(url_);
        connect(proxy,
                &DownloadMediaProxy::fileDownloaded,
                this,
                [proxy, this, filename](const QByteArray &data) {
                        proxy->deleteLater();

                        try {
                                QFile file(filename);

                                if (!file.open(QIODevice::WriteOnly))
                                        return;

                                file.write(data);
                                file.close();
                        } catch (const std::exception &ex) {
                                qDebug() << "Error while saving file to:" << ex.what();
                        }
                });
}
