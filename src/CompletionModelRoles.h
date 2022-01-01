// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractItemModel>

// Interface for completion models
namespace CompletionModel {

// Start at Qt::UserRole * 2 to prevent clashes
enum Roles
{
    CompletionRole = Qt::UserRole * 2, // The string to replace the active completion
    SearchRole,                        // String completer uses for search
    SearchRole2,                       // Secondary string completer uses for search
};
}
