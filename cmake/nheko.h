namespace nheko {
constexpr auto version          = "${PROJECT_VERSION}";
constexpr auto build_user       = "${BUILD_USER}@${BUILD_HOST}";
constexpr auto build_os         = "${CMAKE_HOST_SYSTEM_NAME}";
constexpr auto enable_debug_log = ${SPDLOG_DEBUG_ON};
}
