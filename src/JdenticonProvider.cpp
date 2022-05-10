// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "JdenticonProvider.h"

#include <QApplication>
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

namespace Jdenticon {
JdenticonInterface *
getJdenticonInterface()
{
    static JdenticonInterface *interface = nullptr;
    static bool interfaceExists{true};

    if (interface == nullptr && interfaceExists) {
        QPluginLoader pluginLoader(QStringLiteral("qtjdenticon"));
        QObject *plugin = pluginLoader.instance();
        if (plugin) {
            interface = qobject_cast<JdenticonInterface *>(plugin);
            if (interface) {
                nhlog::ui()->info("Loaded jdenticon plugin.");
            }
        }

        if (!interface) {
            nhlog::ui()->info("jdenticon plugin not found.");
            interfaceExists = false;
        }
    }

    return interface;
}
}

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
{
    auto runnable = new JdenticonRunnable(key, crop, radius, requestedSize);
    connect(runnable, &JdenticonRunnable::done, this, &JdenticonResponse::handleDone);
    QThreadPool::globalInstance()->start(runnable);
}

JdenticonRunnable::JdenticonRunnable(const QString &key,
                                     bool crop,
                                     double radius,
                                     const QSize &requestedSize)
  : m_key(key)
  , m_crop{crop}
  , m_radius{radius}
  , m_requestedSize(requestedSize.isValid() ? requestedSize : QSize(100, 100))
{}

void
JdenticonRunnable::run()
{
    QPixmap pixmap(m_requestedSize);
    pixmap.fill(Qt::transparent);

    auto jdenticon = Jdenticon::getJdenticonInterface();
    if (!jdenticon) {
        emit done(pixmap.toImage());
        return;
    }

    QPainter painter;
    painter.begin(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    try {
        QSvgRenderer renderer{jdenticon->generate(m_key, m_requestedSize.width()).toUtf8()};
        renderer.render(&painter);
    } catch (std::exception &e) {
        nhlog::ui()->error(
          "caught {} in jdenticonprovider, key '{}'", e.what(), m_key.toStdString());
    }

    painter.end();

    pixmap = clipRadius(pixmap, m_radius);

    emit done(pixmap.toImage());
}

bool
JdenticonProvider::isAvailable()
{
    return Jdenticon::getJdenticonInterface() != nullptr;
}
