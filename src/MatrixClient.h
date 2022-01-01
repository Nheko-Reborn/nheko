// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <mtxclient/http/client.hpp>

#include <curl/curl.h>

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

template<>
struct fmt::formatter<mtx::http::ClientError>
{
    // Presentation format: 'f' - fixed, 'e' - exponential.
    bool print_network_error = false;
    bool print_http_error    = false;
    bool print_parser_error  = false;
    bool print_matrix_error  = false;

    // Parses format specifications of the form ['f' | 'e'].
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        // [ctx.begin(), ctx.end()) is a character range that contains a part of
        // the format string starting from the format specifications to be parsed,
        // e.g. in
        //
        //   fmt::format("{:f} - point of interest", point{1, 2});
        //
        // the range will contain "f} - point of interest". The formatter should
        // parse specifiers until '}' or the end of the range. In this example
        // the formatter should parse the 'f' specifier and return an iterator
        // pointing to '}'.

        // Parse the presentation format and store it in the formatter:
        auto it = ctx.begin(), end = ctx.end();

        while (it != end && *it != '}') {
            auto tmp = *it++;

            switch (tmp) {
            case 'n':
                print_matrix_error = true;
                break;
            case 'h':
                print_matrix_error = true;
                break;
            case 'p':
                print_matrix_error = true;
                break;
            case 'm':
                print_matrix_error = true;
                break;
            default:
                throw format_error("invalid format specifier for mtx error");
            }
        }

        // Check if reached the end of the range:
        if (it != end && *it != '}')
            throw format_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }

    // Formats the point p using the parsed format specification (presentation)
    // stored in this formatter.
    template<typename FormatContext>
    auto format(const mtx::http::ClientError &e, FormatContext &ctx) -> decltype(ctx.out())
    {
        // ctx.out() is an output iterator to write to.
        bool prepend_comma = false;
        format_to(ctx.out(), "(");
        if (print_network_error || e.error_code) {
            format_to(ctx.out(), "connection: {}", e.error_code_string());
            prepend_comma = true;
        }

        if (print_http_error ||
            (e.status_code != 0 && (e.status_code < 200 || e.status_code >= 300))) {
            if (prepend_comma)
                format_to(ctx.out(), ", ");
            format_to(ctx.out(), "http: {}", e.status_code);
            prepend_comma = true;
        }

        if (print_parser_error || !e.parse_error.empty()) {
            if (prepend_comma)
                format_to(ctx.out(), ", ");
            format_to(ctx.out(), "parser: {}", e.parse_error);
            prepend_comma = true;
        }

        if (print_parser_error ||
            (e.matrix_error.errcode != mtx::errors::ErrorCode::M_UNRECOGNIZED &&
             !e.matrix_error.error.empty())) {
            if (prepend_comma)
                format_to(ctx.out(), ", ");
            format_to(ctx.out(),
                      "matrix: {}:'{}'",
                      to_string(e.matrix_error.errcode),
                      e.matrix_error.error);
        }

        return format_to(ctx.out(), ")");
    }
};

template<>
struct fmt::formatter<std::optional<mtx::http::ClientError>> : formatter<mtx::http::ClientError>
{
    // parse is inherited from formatter<string_view>.
    template<typename FormatContext>
    auto format(std::optional<mtx::http::ClientError> c, FormatContext &ctx)
    {
        if (!c)
            return format_to(ctx.out(), "(no error)");
        else
            return formatter<mtx::http::ClientError>::format(*c, ctx);
    }
};
