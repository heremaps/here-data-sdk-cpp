[[ -d "build" ]] && rm -rf build
mkdir build && cd build
cmake .. -DCMAKE_CXX_FLAGS=" /MP" \
         -G "Visual Studio 15 2017 Win64" \
         -DCMAKE_BUILD_TYPE=$BUILD_TYPE
cmake --build . --config $BUILD_TYPE
