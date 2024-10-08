// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MatrixClient.h"

#include <memory>

#include <QMetaType>
#include <QObject>
#include <QStandardPaths>
#include <QString>

#include <mtx/responses.hpp>

#include "Logging.h"
#include "UserSettingsPage.h"

namespace http {

mtx::http::Client *
client()
{
    static auto client_ = [] {
        auto c = std::make_shared<mtx::http::Client>();

        // Disabled by default until CPU usage and reliability improves
        if (UserSettings::instance()->qsettings()->value("enable_http3").toBool()) {
            nhlog::net()->warn("Enabling http3 support. This is currently usually a worse "
                               "experience, so you are on your own.");
            c->alt_svc_cache_path((QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
                                   "/curl_alt_svc_cache.txt")
                                    .toStdString());
        }
        return c;
    }();
    return client_.get();
}

bool
is_logged_in()
{
    return !client()->access_token().empty();
}

void
init()
{
}

} // namespace http
