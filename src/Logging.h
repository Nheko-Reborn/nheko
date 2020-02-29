#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace nhlog {
void
init(const std::string &file);

std::shared_ptr<spdlog::logger>
ui();

std::shared_ptr<spdlog::logger>
net();

std::shared_ptr<spdlog::logger>
db();

std::shared_ptr<spdlog::logger>
crypto();

std::shared_ptr<spdlog::logger>
qml();

extern bool enable_debug_log_from_commandline;
}
