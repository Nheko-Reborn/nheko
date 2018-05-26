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
#include <QFile>
#include <QFileDialog>
#include <QPainter>
#include <QPixmap>

#include "MatrixClient.h"
#include "Utils.h"

#include "timeline/widgets/AudioItem.h"

constexpr int MaxWidth          = 400;
constexpr int Height            = 70;
constexpr int IconRadius        = 22;
constexpr int IconDiameter      = IconRadius * 2;
constexpr int HorizontalPadding = 12;
constexpr int TextPadding       = 15;
constexpr int ActionIconRadius  = IconRadius - 4;

constexpr double VerticalPadding = Height - 2 * IconRadius;
constexpr double IconYCenter     = Height / 2;
constexpr double IconXCenter     = HorizontalPadding + IconRadius;

void
AudioItem::init()
{
        setMouseTracking(true);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover, true);

        playIcon_.addFile(":/icons/icons/ui/play-sign.png");
        pauseIcon_.addFile(":/icons/icons/ui/pause-symbol.png");

        QList<QString> url_parts = url_.toString().split("mxc://");
        if (url_parts.size() != 2) {
                qDebug() << "Invalid format for image" << url_.toString();
                return;
        }

        QString media_params = url_parts[1];
        url_                 = QString("%1/_matrix/media/r0/download/%2")
                 .arg(http::client()->getHomeServer().toString(), media_params);

        player_ = new QMediaPlayer;
        player_->setMedia(QUrl(url_));
        player_->setVolume(100);
        player_->setNotifyInterval(1000);

        connect(player_, &QMediaPlayer::stateChanged, this, [this](QMediaPlayer::State state) {
                if (state == QMediaPlayer::StoppedState) {
                        state_ = AudioState::Play;
                        player_->setMedia(QUrl(url_));
                        update();
                }
        });

        setFixedHeight(Height);
}

AudioItem::AudioItem(const mtx::events::RoomEvent<mtx::events::msg::Audio> &event, QWidget *parent)
  : QWidget(parent)
  , url_{QUrl(QString::fromStdString(event.content.url))}
  , text_{QString::fromStdString(event.content.body)}
  , event_{event}
{
        readableFileSize_ = utils::humanReadableFileSize(event.content.info.size);

        init();
}

AudioItem::AudioItem(const QString &url, const QString &filename, uint64_t size, QWidget *parent)
  : QWidget(parent)
  , url_{url}
  , text_{filename}
{
        readableFileSize_ = utils::humanReadableFileSize(size);

        init();
}

QSize
AudioItem::sizeHint() const
{
        return QSize(MaxWidth, Height);
}

void
AudioItem::mousePressEvent(QMouseEvent *event)
{
        if (event->button() != Qt::LeftButton)
                return;

        auto point = event->pos();

        // Click on the download icon.
        if (QRect(HorizontalPadding, VerticalPadding / 2, IconDiameter, IconDiameter)
              .contains(point)) {
                if (state_ == AudioState::Play) {
                        state_ = AudioState::Pause;
                        player_->play();
                } else {
                        state_ = AudioState::Play;
                        player_->pause();
                }

                update();
        } else {
                filenameToSave_ = QFileDialog::getSaveFileName(this, tr("Save File"), text_);

                if (filenameToSave_.isEmpty())
                        return;

                auto proxy = http::client()->downloadFile(url_);
                connect(proxy.data(),
                        &DownloadMediaProxy::fileDownloaded,
                        this,
                        [proxy, this](const QByteArray &data) {
                                proxy->deleteLater();
                                fileDownloaded(data);
                        });
        }
}

void
AudioItem::fileDownloaded(const QByteArray &data)
{
        try {
                QFile file(filenameToSave_);

                if (!file.open(QIODevice::WriteOnly))
                        return;

                file.write(data);
                file.close();
        } catch (const std::exception &ex) {
                qDebug() << "Error while saving file to:" << ex.what();
        }
}

void
AudioItem::resizeEvent(QResizeEvent *event)
{
        QFont font;
        font.setPixelSize(12);
        font.setWeight(80);

        QFontMetrics fm(font);
        const int computedWidth = std::min(
          fm.width(text_) + 2 * IconRadius + VerticalPadding * 2 + TextPadding, (double)MaxWidth);

        resize(computedWidth, Height);

        event->accept();
}

void
AudioItem::paintEvent(QPaintEvent *event)
{
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QFont font;
        font.setPixelSize(12);
        font.setWeight(80);

        QFontMetrics fm(font);

        QPainterPath path;
        path.addRoundedRect(QRectF(0, 0, width(), height()), 10, 10);

        painter.setPen(Qt::NoPen);
        painter.fillPath(path, backgroundColor_);
        painter.drawPath(path);

        QPainterPath circle;
        circle.addEllipse(QPoint(IconXCenter, IconYCenter), IconRadius, IconRadius);

        painter.setPen(Qt::NoPen);
        painter.fillPath(circle, iconColor_);
        painter.drawPath(circle);

        QIcon icon_;
        if (state_ == AudioState::Play)
                icon_ = playIcon_;
        else
                icon_ = pauseIcon_;

        icon_.paint(&painter,
                    QRect(IconXCenter - ActionIconRadius / 2,
                          IconYCenter - ActionIconRadius / 2,
                          ActionIconRadius,
                          ActionIconRadius),
                    Qt::AlignCenter,
                    QIcon::Normal);

        const int textStartX = HorizontalPadding + 2 * IconRadius + TextPadding;
        const int textStartY = VerticalPadding + fm.ascent() / 2;

        // Draw the filename.
        QString elidedText = fm.elidedText(
          text_, Qt::ElideRight, width() - HorizontalPadding * 2 - TextPadding - 2 * IconRadius);

        painter.setFont(font);
        painter.setPen(QPen(textColor_));
        painter.drawText(QPoint(textStartX, textStartY), elidedText);

        // Draw the filesize.
        font.setWeight(50);
        painter.setFont(font);
        painter.setPen(QPen(textColor_));
        painter.drawText(QPoint(textStartX, textStartY + 1.5 * fm.ascent()), readableFileSize_);
}
