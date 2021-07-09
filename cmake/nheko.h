namespace nheko {
constexpr auto version          = "${PROJECT_VERSION}";
constexpr auto build_os         = "${CMAKE_HOST_SYSTEM_NAME}";
constexpr auto enable_debug_log = ${SPDLOG_DEBUG_ON};
}

# cmakedefine01 HAVE_BACKTRACE_SYMBOLS_FD
