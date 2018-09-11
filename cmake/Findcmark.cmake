#
# CMake module to search for the cmark library
#

include(FindPkgConfig)
pkg_check_modules(PC_CMARK QUIET cmark)

if(NOT CMARK_INCLUDE_DIR)
  find_path(CMARK_INCLUDE_DIR
            NAMES cmark.h
            PATHS ${PC_CMARK_INCLUDEDIR}
                  ${PC_CMARK_INCLUDE_DIRS}
                  /usr/include
                  /usr/local/include)
endif()

if(NOT CMARK_LIBRARY)
  find_library(CMARK_LIBRARY
               NAMES cmark
               HINTS ${PC_CMARK_LIBDIR}
                     ${PC_CMARK_LIBRARY_DIRS}
                     /usr/lib
                     /usr/local/lib)
endif()

if(NOT TARGET cmark::cmark)
  add_library(cmark::cmark UNKNOWN IMPORTED)
  set_target_properties(cmark::cmark
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   ${CMARK_INCLUDE_DIR})
  set_property(TARGET cmark::cmark APPEND
               PROPERTY IMPORTED_LOCATION ${CMARK_LIBRARY})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(cmark
                                  DEFAULT_MSG
                                  CMARK_INCLUDE_DIR
                                  CMARK_LIBRARY)

mark_as_advanced(CMARK_LIBRARY CMARK_INCLUDE_DIR)

set(CMARK_LIBRARIES ${CMARK_LIBRARY})
set(CMARK_INCLUDE_DIRS ${CMARK_INCLUDE_DIR})
