// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <mtxclient/http/client.hpp>

#include "Logging.h"

namespace http {
mtx::http::Client *
client();

bool
is_logged_in();

//! Initialize the http module
void
init();
}
