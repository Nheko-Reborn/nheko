ExternalProject_Add(
  Olm

  GIT_REPOSITORY ${OLM_URL}
  GIT_TAG ${OLM_TAG}

  BUILD_IN_SOURCE 1
  SOURCE_DIR ${DEPS_BUILD_DIR}/olm
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/OlmCMakeLists.txt
      ${DEPS_BUILD_DIR}/olm/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/OlmConfig.cmake.in
      ${DEPS_BUILD_DIR}/olm/cmake/OlmConfig.cmake.in
    COMMAND ${CMAKE_COMMAND}
      -DCMAKE_INSTALL_PREFIX=${DEPS_INSTALL_DIR}
      -DCMAKE_BUILD_TYPE=Release
      ${DEPS_BUILD_DIR}/olm
  BUILD_COMMAND ${CMAKE_COMMAND}
    --build ${DEPS_BUILD_DIR}/olm
    --config Release
  INSTALL_COMMAND ${CMAKE_COMMAND}
    --build ${DEPS_BUILD_DIR}/olm
    --target install)

list(APPEND THIRD_PARTY_DEPS Olm)
