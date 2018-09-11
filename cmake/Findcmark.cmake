#
# CMake module to search for the cmark library
#

find_path(CMARK_INCLUDE_DIR
          NAMES cmark.h
          PATHS /usr/include
                /usr/local/include
                $ENV{LIB_DIR}/include
                $ENV{LIB_DIR}/include/cmark)

find_library(CMARK_LIBRARY
             NAMES cmark
             PATHS /usr/lib /usr/local/lib $ENV{LIB_DIR}/lib)

if(OLM_FOUND)
  set(OLM_INCLUDE_DIRS ${CMARK_INCLUDE_DIR})

  if(NOT OLM_LIBRARIES)
    set(OLM_LIBRARIES ${CMARK_LIBRARY})
  endif()
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
