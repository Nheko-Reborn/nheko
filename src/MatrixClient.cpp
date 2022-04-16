// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MatrixClient.h"

#include <memory>
#include <set>

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
} // namespace http
