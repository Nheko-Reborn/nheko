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
        QPainter painter{&m_pixmap};
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        QPainterPath ppath;
        ppath.addRoundedRect(m_pixmap.rect(), m_radius, m_radius);

        painter.setClipPath(ppath);

        QSvgRenderer renderer{
          jdenticonInterface_->generate(m_key, m_requestedSize.width()).toUtf8()};
        renderer.render(&painter);

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
                        for (QString fileName : pluginsDir.entryList(QDir::Files)) {
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
