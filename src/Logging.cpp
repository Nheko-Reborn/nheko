// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Logging.h"
#include "config/nheko.h"

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <iostream>

#include <QString>
#include <QtGlobal>

#include <mtx/log.hpp>

namespace {
std::shared_ptr<spdlog::logger> db_logger     = nullptr;
std::shared_ptr<spdlog::logger> net_logger    = nullptr;
std::shared_ptr<spdlog::logger> crypto_logger = nullptr;
std::shared_ptr<spdlog::logger> ui_logger     = nullptr;
std::shared_ptr<spdlog::logger> qml_logger    = nullptr;

constexpr auto MAX_FILE_SIZE = 1024 * 1024 * 6;
constexpr auto MAX_LOG_FILES = 3;

void
qmlMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    std::string localMsg = msg.toStdString();
    const char *file     = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";

    if (
      // The default style has the point size set. If you use pixel size anywhere, you get
      // that warning, which is useless, since sometimes you need the pixel size to match the
      // text to the size of the outer element for example. This is done in the avatar and
      // without that you get one warning for every Avatar displayed, which is stupid!
      msg.endsWith(QStringLiteral("Both point size and pixel size set. Using pixel size.")))
        return;

    switch (type) {
    case QtDebugMsg:
        nhlog::qml()->debug("{} ({}:{}, {})", localMsg, file, context.line, function);
        break;
    case QtInfoMsg:
        nhlog::qml()->info("{} ({}:{}, {})", localMsg, file, context.line, function);
        break;
    case QtWarningMsg:
        nhlog::qml()->warn("{} ({}:{}, {})", localMsg, file, context.line, function);
        break;
    case QtCriticalMsg:
        nhlog::qml()->critical("{} ({}:{}, {})", localMsg, file, context.line, function);
        break;
    case QtFatalMsg:
        nhlog::qml()->critical("{} ({}:{}, {})", localMsg, file, context.line, function);
        break;
    }
}
}

namespace nhlog {
bool enable_debug_log_from_commandline = false;

void
init(const std::string &file_path)
{
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
      file_path, MAX_FILE_SIZE, MAX_LOG_FILES);

    auto console_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();

    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(file_sink);
    sinks.push_back(console_sink);

    mtx::utils::log::log()->sinks() = sinks;
    net_logger    = std::make_shared<spdlog::logger>("net", std::begin(sinks), std::end(sinks));
    ui_logger     = std::make_shared<spdlog::logger>("ui", std::begin(sinks), std::end(sinks));
    db_logger     = std::make_shared<spdlog::logger>("db", std::begin(sinks), std::end(sinks));
    crypto_logger = std::make_shared<spdlog::logger>("crypto", std::begin(sinks), std::end(sinks));
    qml_logger    = std::make_shared<spdlog::logger>("qml", std::begin(sinks), std::end(sinks));

    if (nheko::enable_debug_log || enable_debug_log_from_commandline) {
        db_logger->set_level(spdlog::level::trace);
        ui_logger->set_level(spdlog::level::trace);
        crypto_logger->set_level(spdlog::level::trace);
        net_logger->set_level(spdlog::level::trace);
        qml_logger->set_level(spdlog::level::trace);
        mtx::utils::log::log()->set_level(spdlog::level::trace);
    }

    qInstallMessageHandler(qmlMessageHandler);
}

std::shared_ptr<spdlog::logger>
ui()
{
    return ui_logger;
}

std::shared_ptr<spdlog::logger>
net()
{
    return net_logger;
}

std::shared_ptr<spdlog::logger>
db()
{
    return db_logger;
}

std::shared_ptr<spdlog::logger>
crypto()
{
    return crypto_logger;
}

std::shared_ptr<spdlog::logger>
qml()
{
    return qml_logger;
}
}
