include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        6d668180665e313bbda19f8187abf6140e80a7d7
    GIT_PROGRESS   TRUE
)

FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)	
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_Populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif()
