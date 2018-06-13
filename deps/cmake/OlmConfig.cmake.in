get_filename_component(Olm_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

list(APPEND CMAKE_MODULE_PATH ${Olm_CMAKE_DIR})
list(REMOVE_AT CMAKE_MODULE_PATH -1)

if(NOT TARGET Olm::olm)
  include("${Olm_CMAKE_DIR}/OlmTargets.cmake")
endif()

set(Olm_LIBRARIES Olm::olm)
