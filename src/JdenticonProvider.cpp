// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "JdenticonProvider.h"

#include <QApplication>
#include <QDir>
#include <QPainter>
#include <QPainterPath>
#include <QPluginLoader>
#include <QSvgRenderer>

#include <mtxclient/crypto/client.hpp>

#include "Cache.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "Utils.h"
#include "jdenticoninterface.h"

static QPixmap
clipRadius(QPixmap img, double radius)
{
        QPixmap out(img.size());
        out.fill(Qt::transparent);

        QPainter painter(&out);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        QPainterPath ppath;
        ppath.addRoundedRect(img.rect(), radius, radius, Qt::SizeMode::RelativeSize);

        painter.setClipPath(ppath);
        painter.drawPixmap(img.rect(), img);

        return out;
}

JdenticonResponse::JdenticonResponse(const QString &key,
                                     bool crop,
                                     double radius,
                                     const QSize &requestedSize)
  : m_key(key)
  , m_crop{crop}
  , m_radius{radius}
  , m_requestedSize(requestedSize.isValid() ? requestedSize : QSize(100, 100))
  , m_pixmap{m_requestedSize}
  , jdenticonInterface_{Jdenticon::getJdenticonInterface()}
{
        setAutoDelete(false);
}

void
JdenticonResponse::run()
{
        m_pixmap.fill(Qt::transparent);

        QPainter painter;
        painter.begin(&m_pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        QSvgRenderer renderer{
          jdenticonInterface_->generate(m_key, m_requestedSize.width()).toUtf8()};
        renderer.render(&painter);

        painter.end();

        m_pixmap = clipRadius(m_pixmap, m_radius);

        emit finished();
}

namespace Jdenticon {
JdenticonInterface *
getJdenticonInterface()
{
        static JdenticonInterface *interface = nullptr;

        if (interface == nullptr) {
                QDir pluginsDir(qApp->applicationDirPath());

                bool plugins = pluginsDir.cd("plugins");
                if (plugins) {
                        for (const QString &fileName : pluginsDir.entryList(QDir::Files)) {
                                QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
                                QObject *plugin = pluginLoader.instance();
                                if (plugin) {
                                        interface = qobject_cast<JdenticonInterface *>(plugin);
                                        if (interface) {
                                                nhlog::ui()->info("Loaded jdenticon plugin.");
                                                break;
                                        }
                                }
                        }
                } else {
                        nhlog::ui()->info("jdenticon plugin not found.");
                }
        }

        return interface;
}
}
