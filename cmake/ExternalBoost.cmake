include(FetchContent)

FetchContent_Declare(
    boost
    URL            https://sourceforge.net/projects/boost/files/boost/1.69.0/boost_1_69_0.tar.gz/download
    GIT_PROGRESS   TRUE
)

FetchContent_GetProperties(boost)
if(NOT boost_POPULATED)	
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_Populate(boost)
endif()

set(BOOST_ROOT ${boost_SOURCE_DIR})
set(Boost_INCLUDE_DIR ${boost_SOURCE_DIR})
set(Boost_LIBRARY_DIR ${boost_BINARY_DIR})
