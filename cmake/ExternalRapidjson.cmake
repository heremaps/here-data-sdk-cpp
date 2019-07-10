include(FetchContent)

FetchContent_Declare(rapidjson
    GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
    GIT_SUBMODULES "test"
)

FetchContent_GetProperties(rapidjson)
if(NOT rapidjson_POPULATED)
    set(RAPIDJSON_BUILD_DOC OFF)
    set(RAPIDJSON_BUILD_EXAMPLES OFF)
    set(RAPIDJSON_BUILD_TESTS OFF)
    FetchContent_Populate(rapidjson)
    add_subdirectory(${rapidjson_SOURCE_DIR} ${rapidjson_BINARY_DIR})
endif()
