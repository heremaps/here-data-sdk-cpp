include(FetchContent)

FetchContent_Declare(
    snappy
    GIT_REPOSITORY     https://github.com/google/snappy.git
    GIT_TAG            1.1.7
)

FetchContent_GetProperties(snappy)

if(NOT snappy_POPULATED)
    FetchContent_Populate(snappy)
    add_subdirectory(${snappy_SOURCE_DIR} ${snappy_BINARY_DIR})
endif()

FetchContent_Declare(
    leveldb
    GIT_REPOSITORY     https://github.com/google/leveldb.git
    GIT_TAG            1.21
)

FetchContent_GetProperties(leveldb)

if(NOT leveldb_POPULATED)
    FetchContent_Populate(leveldb)
    set(LEVELDB_BUILD_TESTS OFF)
    set(LEVELDB_BUILD_BENCHMARKS OFF)
    set(LEVELDB_INSTALL OFF)
    add_subdirectory(${leveldb_SOURCE_DIR} ${leveldb_BINARY_DIR})
endif()

add_library(leveldb::leveldb ALIAS leveldb)
