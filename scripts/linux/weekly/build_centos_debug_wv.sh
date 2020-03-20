#!/bin/bash -ex
#
# Copyright (C) 2020 HERE Europe B.V.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
# License-Filename: LICENSE

#
# Special build in debug, to be build in special docker images: Centos gcc 7.3
#

# Set workspace location
if [[ ${WORKSPACE} == "" ]]; then
    export WORKSPACE=`pwd`
fi
# Add all needed variables
export BUILD_DIR_NAME="build"
export BUILD_DIR="$WORKSPACE/$BUILD_DIR_NAME"
export ARCHIVE_FILE_NAME="binaries.tar.gz"      # artifact name is defined by CI so please do not rename it
export BUILD_ZIP=1                              # always build artifacts

export CMAKE_CXX_FLAGS="-march=x86-64 -gdwarf-2 -g3 -O0 -fno-builtin -Wall -Wextra -Werror -Wno-deprecated-declarations -Wno-unused-parameter -Wno-unused-function -Wno-attributes"
export CMAKE_C_FLAGS="-march=x86-64 -gdwarf-2 -g3 -O0 -fno-builtin -Wall -Wextra -Werror -Wno-deprecated-declarations -Wno-unused-parameter -Wno-unused-function -Wno-attributes"

# Add more parameters into CMAKE_PARAM below when needed
export FLAVOR="Debug"
export CMAKE_PARAM="-DCMAKE_CXX_FLAGS=\"${CMAKE_CXX_FLAGS}\" -DCMAKE_C_FLAGS=\"${CMAKE_C_FLAGS}\" -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=$FLAVOR -DOLP_SDK_BUILD_EXAMPLES=ON"
export CMAKE_COMMAND="cmake ${CMAKE_PARAM} .."
export CMAKE_BUILD_ALL_TARGETS="make -j$(nproc)"

# Function :
build_for_centos() {
    echo ""
    echo ""
    echo "*************** $VARIANT Build EDGE SDK CPP ********** Start ***************"
    # Show initial ccache data
    ccache -s
    echo ""
    echo " ---- Calling ${CMAKE_COMMAND}"
    eval "${CMAKE_COMMAND}"

    # Run CMake. Warnings and errors are saved to build/CMakeFiles/CMakeOutput.log and
    # build/CMakeFiles/CMakeError.log.
    # -- We link Edge SDK as shared libraries in order to use shadowing for unit tests.
    # -- We build the examples.
    echo ""
    echo " ---- Calling ${CMAKE_BUILD_ALL_TARGETS}"
    ${CMAKE_BUILD_ALL_TARGETS}

    cd ${WORKSPACE}

    if [[ ${BUILD_ZIP} -eq 1 ]]; then
        echo "Zipping up artifacts needed for testing ..."

        # Prepare artifacts archive for test job
        # Zip up all the binaries needed to run the tests but need to leave the tar.gz file in build
        # folder as this ensure the CI system archives it automatically
        tar -czf "${BUILD_DIR_NAME}/$ARCHIVE_FILE_NAME" --exclude='*.git' --wildcards ${BUILD_DIR_NAME}/olp-*/*.so
        # List files in build archive
        tar -tvf "${BUILD_DIR_NAME}/$ARCHIVE_FILE_NAME"
    fi
    echo ""
    echo ""
    echo "*************** $VARIANT Build EDGE SDK CPP ********** Done ***************"
}

while [[ $# -gt 0 ]]; do
    key="$1"
    case "$key" in
        -c|--centos)
        BUILD_VARIANT=1
        VARIANT="Centos"
        ;;
    esac
    # Shift after checking all the cases to get the next option
    shift
done

    if [[ -d ${BUILD_DIR} ]]; then
        rm -rf ${BUILD_DIR}
    fi
    mkdir -p ${BUILD_DIR}

    echo ""
    echo "*************** Build Started ***************"
    echo "WORKSPACE: $WORKSPACE"
    echo "BUILD DIR: $BUILD_DIR"
    echo "FLAVOR: $FLAVOR"
    echo "VARIANT: $VARIANT"
    echo ""
    echo ""

    # Goto build folder
    cd "$BUILD_DIR"

    case "$BUILD_VARIANT" in
        1)
            build_for_centos
            ;;
    esac
    echo ""
    echo ""
    echo "*************** Build Done ***************"

