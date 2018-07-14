#pragma once

#include <QMetaType>
#include <QString>

#include <mtx/responses.hpp>
#include <mtxclient/http/client.hpp>

Q_DECLARE_METATYPE(mtx::responses::Login)
Q_DECLARE_METATYPE(mtx::responses::Messages)
Q_DECLARE_METATYPE(mtx::responses::Notifications)
Q_DECLARE_METATYPE(mtx::responses::Rooms)
Q_DECLARE_METATYPE(mtx::responses::Sync)
Q_DECLARE_METATYPE(mtx::responses::JoinedGroups)
Q_DECLARE_METATYPE(mtx::responses::GroupProfile)
Q_DECLARE_METATYPE(std::string)
Q_DECLARE_METATYPE(std::vector<std::string>)
Q_DECLARE_METATYPE(std::vector<QString>)

namespace http {
namespace v2 {
mtx::http::Client *
client();

bool
is_logged_in();
}

//! Initialize the http module
void
init();
}
