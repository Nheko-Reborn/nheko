// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QRegularExpression>
#include <QString>

// clazy:excludeall=non-pod-global-static

// Non-theme app configuration. Layouts, fonts spacing etc.
//
// Font sizes are in pixels.
namespace conf {

namespace modals {
inline constexpr int WIDGET_MARGIN  = 20;
inline constexpr int WIDGET_SPACING = 15;

inline constexpr auto LABEL_MEDIUM_SIZE_RATIO = 1.3;
}

namespace strings {
inline const QString url_html = QStringLiteral("<a href=\"\\1\">\\1</a>");
inline const QRegularExpression url_regex(
  // match an unquoted URL
  []() {
      const auto general_unicode = QStringLiteral(
        R"((?:[^\x{0}-\x{7f}\p{Cc}\s\p{P}]|[\x{2010}\x{2011}\x{2012}\x{2013}\x{2014}\x{2015}]))");
      const auto protocol                   = QStringLiteral(R"((?:[Hh][Tt][Tt][Pp][Ss]?))");
      const auto unreserved_subdelims_colon = QStringLiteral(R"([a-zA-Z0-9\-._~!$&'()*+,;=:])");
      const auto pct_enc                    = QStringLiteral(R"((?:%[[:xdigit:]]{2}))");
      const auto userinfo =
        "(?:" + unreserved_subdelims_colon + "*(?:" + pct_enc + unreserved_subdelims_colon + "*)*)";
      const auto dec_octet =
        QStringLiteral(R"((?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9]))");
      const auto ipv4_addr = "(?:" + dec_octet + R"((?:\.)" + dec_octet + "){3})";
      const auto h16       = QStringLiteral(R"((?:[[:xdigit:]]{1,4}))");
      const auto ls32      = "(?:" + h16 + ":" + h16 + "|" + ipv4_addr + ")";
      // clang-format off
        const auto ipv6_addr = "(?:"
                               "(?:" + h16 + ":){6}" + ls32
                               + "|" "::(?:" + h16 + ":){5}" + ls32
                               + "|" + h16 + "?::(?:" + h16 + ":){4}" + ls32
                               + "|" "(?:" + h16 + "(?::" + h16 + "){0,1})?::(?:" + h16 + ":){3}" + ls32
                               + "|" "(?:" + h16 + "(?::" + h16 + "){0,2})?::(?:" + h16 + ":){2}" + ls32
                               + "|" "(?:" + h16 + "(?::" + h16 + "){0,3})?::" + h16 + ":" + ls32
                               + "|" "(?:" + h16 + "(?::" + h16 + "){0,4})?::" + ls32
                               + "|" "(?:" + h16 + "(?::" + h16 + "){0,5})?::" + h16
                               + "|" "(?:" + h16 + "(?::" + h16 + "){0,6})?::"
                               ")";
      // clang-format on
      const auto ipvfuture  = R"((?:v[[:xdigit:]]+\.)" + unreserved_subdelims_colon + "+)";
      const auto ip_literal = R"((?:\[(?:)" + ipv6_addr + "|" + ipvfuture + R"()\]))";
      const auto host_alnum = "(?:[a-zA-Z0-9]|" + general_unicode + ")";
      const auto host_label = "(?:" + host_alnum + "+(?:-+" + host_alnum + "+)*)";
      const auto hostname   = "(?:" + host_label + R"((?:\.)" + host_label + R"()*\.?))";
      const auto host       = "(?:" + hostname + "|" + ip_literal + ")";
      const auto path = R"((?:/((?:[a-zA-Z0-9\-._~!$&'*+,;=:@/]|)" + pct_enc + R"(|\((?-1)\)|)" +
                        general_unicode + ")*))";
      const auto query = R"(((?:[a-zA-Z0-9\-._~!$&'*+,;=:@/?\\{}]|)" + pct_enc +
                         R"(|\((?-1)\)|\[(?-1)\]|)" + general_unicode + ")*)";
      const auto &fragment = query;
      return R"((?<!["'\w])(?>()" + protocol + "://" + "(?:" + userinfo + "@)?" + host +
             "(?::[0-9]+)?" + path +
             "?"
             R"((?:\?)" +
             query +
             ")?"
             R"((?:#)" +
             fragment +
             ")?"
             "(?<![.!?,;:'])"
             R"())(?!["']))";
  }(),
  QRegularExpression::UseUnicodePropertiesOption);
// A matrix link to be converted back to markdown
inline const QRegularExpression
  matrixToLink(QStringLiteral(R"(<a href=\"(https://matrix.to/#/.*?)\">(.*?)</a>)"));
}

// Window geometry.
namespace window {
inline constexpr int height = 600;
inline constexpr int width  = 1066;

inline constexpr int minHeight = 340;
inline constexpr int minWidth  = 340;
} // namespace window

} // namespace conf
