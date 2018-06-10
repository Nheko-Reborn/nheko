#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace log {
void
init(const std::string &file);

std::shared_ptr<spdlog::logger>
main();

std::shared_ptr<spdlog::logger>
net();

std::shared_ptr<spdlog::logger>
db();

std::shared_ptr<spdlog::logger>
crypto();
}
