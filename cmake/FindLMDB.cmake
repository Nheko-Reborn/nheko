#
# Find the lmdb library & include dir.
#

find_path (LMDB_INCLUDE_DIR NAMES lmdb.h PATHS "$ENV{LMDB_DIR}/include")
find_library (LMDB_LIBRARY NAMES lmdb PATHS "$ENV{LMDB_DIR}/lib" )
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LMDB DEFAULT_MSG LMDB_INCLUDE_DIR LMDB_LIBRARY)


add_library(lmdb INTERFACE IMPORTED GLOBAL)
target_include_directories(lmdb INTERFACE ${LMDB_INCLUDE_DIR})
target_link_libraries(lmdb INTERFACE ${LMDB_LIBRARY})

add_library(liblmdb::lmdb ALIAS lmdb)
