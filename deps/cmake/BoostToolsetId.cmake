# - Translate CMake compilers to the Boost.Build toolset equivalents
# To build Boost reliably when a non-system compiler may be used, we
# need to both specify the toolset when running bootstrap.sh *and* in
# the user-config.jam file.
#
# This module provides the following functions to help translate between
# the systems:
#
#  function Boost_Get_ToolsetId(<var>)
#           Set var equal to Boost's name for the CXX toolchain picked
#           up by CMake. Only supports GNU and Clang families at present.
#           Intel support is provisional
#
# downloaded from https://github.com/drbenmorgan/BoostBuilder/blob/master/BoostToolsetId.cmake

function(Boost_Get_ToolsetId _var)
  set(BOOST_TOOLSET)

  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if(APPLE)
      set(BOOST_TOOLSET "darwin")
    else()
      set(BOOST_TOOLSET "gcc")
    endif()
  elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    set(BOOST_TOOLSET "clang")
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    set(BOOST_TOOLSET "intel")
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(BOOST_TOOLSET "msvc")
  endif()

  set(${_var} ${BOOST_TOOLSET} PARENT_SCOPE)
endfunction()

