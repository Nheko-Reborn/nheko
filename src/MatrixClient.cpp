#include "MatrixClient.h"

#include <memory>

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
}

} // namespace http
