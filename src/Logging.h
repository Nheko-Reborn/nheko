// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string>

#include <QString>

#include "spdlog/logger.h"

namespace nhlog {
void
init(const QString &level, const QString &path, bool to_stderr);

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

}
