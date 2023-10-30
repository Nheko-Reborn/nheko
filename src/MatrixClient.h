// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <mtxclient/http/client.hpp>

namespace http {
mtx::http::Client *
client();

bool
is_logged_in();

//! Initialize the http module
void
init();
}
