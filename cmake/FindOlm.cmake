#
# CMake module to search for the olm library
#
# On success, the macro sets the following variables:
# OLM_FOUND       = if the library found
# OLM_LIBRARY     = full path to the library
# OLM_INCLUDE_DIR = where to find the library headers
#

find_path(OLM_INCLUDE_DIR
          NAMES olm/olm.h
          PATHS /usr/include
                /usr/local/include
                $ENV{LIB_DIR}/include
                $ENV{LIB_DIR}/include/olm)

find_library(OLM_LIBRARY
             NAMES olm
             PATHS /usr/lib /usr/local/lib $ENV{LIB_DIR}/lib)

if(OLM_FOUND)
  set(OLM_INCLUDE_DIRS ${OLM_INCLUDE_DIR})

  if(NOT OLM_LIBRARIES)
    set(OLM_LIBRARIES ${OLM_LIBRARY})
  endif()
endif()

if(NOT TARGET Olm::Olm)
  add_library(Olm::Olm UNKNOWN IMPORTED)
  set_target_properties(Olm::Olm
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   ${OLM_INCLUDE_DIR})
  set_property(TARGET Olm::Olm APPEND PROPERTY IMPORTED_LOCATION ${OLM_LIBRARY})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OLM DEFAULT_MSG OLM_INCLUDE_DIR OLM_LIBRARY)

mark_as_advanced(OLM_LIBRARY OLM_INCLUDE_DIR)
