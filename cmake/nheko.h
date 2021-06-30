namespace nheko {
constexpr auto version          = "${PROJECT_VERSION}";
constexpr auto build_os         = "${CMAKE_HOST_SYSTEM_NAME}";
constexpr auto enable_debug_log = ${SPDLOG_DEBUG_ON};
}

// clang-format off
#define HAVE_BACKTRACE_SYMBOLS_FD ${HAVE_BACKTRACE_SYMBOLS_FD}
// clang-format on
