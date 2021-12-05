#include "nheko_version.h"

namespace nheko {
constexpr auto build_os = "@HOST_SYSTEM_NAME@";
// clang-format off
constexpr auto enable_debug_log = @SPDLOG_DEBUG_ON@;
}

#mesondefine HAVE_BACKTRACE_SYMBOLS_FD
