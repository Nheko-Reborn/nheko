#include "MatrixClient.h"

#include <memory>

namespace {
auto v2_client_ = std::make_shared<mtx::http::Client>("matrix.org");
}

namespace http {
namespace v2 {

mtx::http::Client *
client()
{
        return v2_client_.get();
}

} // namespace v2

void
init()
{
        qRegisterMetaType<mtx::responses::Login>();
        qRegisterMetaType<mtx::responses::Messages>();
        qRegisterMetaType<mtx::responses::Notifications>();
        qRegisterMetaType<mtx::responses::Rooms>();
        qRegisterMetaType<mtx::responses::Sync>();
        qRegisterMetaType<std::string>();
        qRegisterMetaType<std::vector<std::string>>();
}

} // namespace http
