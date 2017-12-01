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

#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>

#include "timeline/widgets/VideoItem.h"

namespace events = matrix::events;
namespace msgs   = matrix::events::messages;

void
VideoItem::init()
{
        QList<QString> url_parts = url_.toString().split("mxc://");
        if (url_parts.size() != 2) {
                qDebug() << "Invalid format for image" << url_.toString();
                return;
        }

        QString media_params = url_parts[1];
        url_                 = QString("%1/_matrix/media/r0/download/%2")
                 .arg(client_.data()->getHomeServer().toString(), media_params);
}

VideoItem::VideoItem(QSharedPointer<MatrixClient> client,
                     const events::MessageEvent<msgs::Video> &event,
                     QWidget *parent)
  : QWidget(parent)
  , url_{event.msgContent().url()}
  , text_{event.content().body()}
  , event_{event}
  , client_{client}
{
        readableFileSize_ = calculateFileSize(event.msgContent().info().size);

        init();

        auto layout = new QVBoxLayout(this);
        layout->setMargin(0);
        layout->setSpacing(0);

        QString link = QString("Video - <a href=%1>%2</a>").arg(url_.toString()).arg(text_);

        label_ = new QLabel(link, this);
        label_->setMargin(0);
        label_->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextBrowserInteraction);
        label_->setOpenExternalLinks(true);

        layout->addWidget(label_);
}

VideoItem::VideoItem(QSharedPointer<MatrixClient> client,
                     const QString &url,
                     const QString &filename,
                     QWidget *parent)
  : QWidget(parent)
  , url_{url}
  , text_{QFileInfo(filename).fileName()}
  , client_{client}
{
        readableFileSize_ = calculateFileSize(QFileInfo(filename).size());

        init();
}

QString
VideoItem::calculateFileSize(int nbytes) const
{
        if (nbytes == 0)
                return QString("");

        if (nbytes < 1024)
                return QString("%1 B").arg(nbytes);

        if (nbytes < 1024 * 1024)
                return QString("%1 KB").arg(nbytes / 1024);

        return QString("%1 MB").arg(nbytes / 1024 / 1024);
}
