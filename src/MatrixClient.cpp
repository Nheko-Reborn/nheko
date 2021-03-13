// SPDX-FileCopyrightText: 2021 Nheko Contributors
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
Q_DECLARE_METATYPE(mtx::responses::JoinedGroups)
Q_DECLARE_METATYPE(mtx::responses::GroupProfile)

Q_DECLARE_METATYPE(nlohmann::json)
Q_DECLARE_METATYPE(std::string)
Q_DECLARE_METATYPE(std::vector<std::string>)
Q_DECLARE_METATYPE(std::vector<QString>)
Q_DECLARE_METATYPE(std::set<QString>)

namespace {
auto client_ = std::make_shared<mtx::http::Client>();
}

namespace http {

mtx::http::Client *
client()
{
        return client_.get();
}

bool
is_logged_in()
{
        return !client_->access_token().empty();
}

void
init()
{
        qRegisterMetaType<mtx::responses::Login>();
        qRegisterMetaType<mtx::responses::Messages>();
        qRegisterMetaType<mtx::responses::Notifications>();
        qRegisterMetaType<mtx::responses::Rooms>();
        qRegisterMetaType<mtx::responses::Sync>();
        qRegisterMetaType<mtx::responses::JoinedGroups>();
        qRegisterMetaType<mtx::responses::GroupProfile>();
        qRegisterMetaType<std::string>();
        qRegisterMetaType<nlohmann::json>();
        qRegisterMetaType<std::vector<std::string>>();
        qRegisterMetaType<std::vector<QString>>();
        qRegisterMetaType<std::map<QString, bool>>("std::map<QString, bool>");
        qRegisterMetaType<std::set<QString>>();
}

} // namespace http
