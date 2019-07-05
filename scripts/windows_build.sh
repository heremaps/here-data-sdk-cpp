mkdir build && cd build
cmake .. -G "Visual Studio 15 2017 Win64" \
         -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
         -DBoost_LIBRARY_DIR_DEBUG="$WORKSPACE/build/external/boost/build/$BUILD_TYPE" \
         -DBoost_ARCHITECTURE=x64
cmake --build . --config $BUILD_TYPE
