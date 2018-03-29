#
# Find the lmdb library & include dir.
# Build lmdb on Appveyor.
#

if(APPVEYOR_BUILD)
    set(LMDB_VERSION "LMDB_0.9.21")
    set(NTDLIB "C:/WINDDK/7600.16385.1/lib/win7/amd64/ntdll.lib")

    execute_process(
        COMMAND git clone --depth=1 --branch ${LMDB_VERSION} https://github.com/LMDB/lmdb)

    set(LMDB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/lmdb/libraries/liblmdb)

    add_library(lmdb
        ${CMAKE_SOURCE_DIR}/lmdb/libraries/liblmdb/lmdb.h
        ${CMAKE_SOURCE_DIR}/lmdb/libraries/liblmdb/mdb.c
        ${CMAKE_SOURCE_DIR}/lmdb/libraries/liblmdb/midl.h
        ${CMAKE_SOURCE_DIR}/lmdb/libraries/liblmdb/midl.c)

    set(LMDB_LIBRARY lmdb)
else()
    find_path (LMDB_INCLUDE_DIR NAMES lmdb.h PATHS "$ENV{LMDB_DIR}/include")
    find_library (LMDB_LIBRARY NAMES lmdb PATHS "$ENV{LMDB_DIR}/lib" )
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(LMDB DEFAULT_MSG LMDB_INCLUDE_DIR LMDB_LIBRARY)
endif()

include_directories(${LMDB_INCLUDE_DIR})
