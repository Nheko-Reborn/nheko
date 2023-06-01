// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MatrixClient.h"

#include <memory>
#include <set>

#include <QMetaType>
#include <QObject>
#include <QStandardPaths>
#include <QString>

#include "nlohmann/json.hpp"
#include <mtx/responses.hpp>

namespace http {

mtx::http::Client *
client()
{
    static auto client_ = [] {
        auto c = std::make_shared<mtx::http::Client>();
        c->alt_svc_cache_path((QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
                               "/curl_alt_svc_cache.txt")
                                .toStdString());
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
