// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CacheCryptoStructs.h"

#include <mtx/events/encrypted.hpp>

MegolmSessionIndex::MegolmSessionIndex(std::string room_id_, const mtx::events::msg::Encrypted &e)
  : room_id(std::move(room_id_))
  , session_id(e.session_id)
{
}

#include "moc_CacheCryptoStructs.cpp"
