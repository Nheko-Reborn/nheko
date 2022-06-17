// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MatrixClient.h"

#include <memory>
#include <set>

#include <QMetaType>
#include <QObject>
#include <QString>

#include "nlohmann/json.hpp"
#include <mtx/responses.hpp>

Q_DECLARE_METATYPE(mtx::responses::Login)
Q_DECLARE_METATYPE(mtx::responses::Messages)
Q_DECLARE_METATYPE(mtx::responses::Notifications)
Q_DECLARE_METATYPE(mtx::responses::Rooms)
Q_DECLARE_METATYPE(mtx::responses::Sync)
Q_DECLARE_METATYPE(mtx::responses::StateEvents)

// Q_DECLARE_METATYPE(nlohmann::json)
Q_DECLARE_METATYPE(std::string)
Q_DECLARE_METATYPE(std::vector<std::string>)
Q_DECLARE_METATYPE(std::vector<QString>)
Q_DECLARE_METATYPE(std::set<QString>)

namespace http {

mtx::http::Client *
client()
{
    static auto client_ = std::make_shared<mtx::http::Client>();
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
    qRegisterMetaType<mtx::responses::Login>();
    qRegisterMetaType<mtx::responses::Messages>();
    qRegisterMetaType<mtx::responses::Notifications>();
    qRegisterMetaType<mtx::responses::Rooms>();
    qRegisterMetaType<mtx::responses::Sync>();
    qRegisterMetaType<mtx::responses::StateEvents>();
    qRegisterMetaType<std::string>();
    // qRegisterMetaType<nlohmann::json>();
    qRegisterMetaType<std::vector<std::string>>();
    qRegisterMetaType<std::vector<QString>>();
    qRegisterMetaType<std::map<QString, bool>>("std::map<QString, bool>");
    qRegisterMetaType<std::set<QString>>();
}

} // namespace http
