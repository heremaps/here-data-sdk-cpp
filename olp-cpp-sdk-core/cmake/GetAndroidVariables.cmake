# Copyright (C) 2019 HERE Europe B.V.
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

# Common functions that resolve Android related variables and
# resolve Android toolchain configuration variables.

function(get_android_sdk_root VAR)
  # If not already set, sets the given VAR output variable to point to the Android SDK's root.
  # Sends a FATAL_ERROR if VAR is not set and cannot be be resolved.
  if(${VAR})
    return()
  endif()

  if(DEFINED ENV{SDK_ROOT})
    set(${VAR} $ENV{SDK_ROOT} PARENT_SCOPE)
  elseif(DEFINED ENV{ANDROID_HOME})
    set(${VAR} $ENV{ANDROID_HOME} PARENT_SCOPE)
  elseif(DEFINED ENV{ANDROID_SDK_ROOT})
    set(${VAR} $ENV{ANDROID_SDK_ROOT} PARENT_SCOPE)
  else()
    message(FATAL_ERROR "Neither SDK_ROOT, ANDROID_HOME nor ANDROID_SDK_ROOT variable found.")
  endif()

endfunction()

function(get_android_platform VAR)
  # If not already set, sets the given output variable VAR, which will then contain an acceptable
  # value for the Android SDK's CMake configuration variable ANDROID_PLATFORM.
  # Sends a FATAL_ERROR if ANDROID_PLATFORM is not set and cannot be resolved.
  if(${VAR})
    return()
  endif()

  if(ANDROID_SDK_VERSION)
    set(${VAR} "android-${ANDROID_SDK_VERSION}" PARENT_SCOPE)
  else()
    message(FATAL_ERROR "Could not determine Android platform since ANDROID_SDK_VERSION is not set")
  endif()
endfunction()

function(get_android_jar_path VAR sdk_root android_platform)
  # If not already set, sets the output variale VAR with help of the given the input variables
  # sdk_root and android_platform to the path of the according android.jar file.
  # sdk_root should point to a valid Android Home path. If not set, it will be resolved internally.
  # android_platform should contain a value that conforms to the Android SDK's CMake config variable
  # with the same name. If not set, it will be resolved internally.
  # May send FATAL_ERROR if the path of the android.jar could not be resolved or the file does not
  # exist.
  if(${VAR})
    if(NOT EXISTS ${${VAR}})
      message(FATAL_ERROR "The file ${${VAR}} does not exist")
    endif()
    return()
  endif()

  get_android_sdk_root(${sdk_root})
  get_android_platform(${android_platform})
  set(android_jar ${${sdk_root}}/platforms/${${android_platform}}/android.jar)
  set(${VAR} ${android_jar} PARENT_SCOPE)
  if(NOT EXISTS ${android_jar})
    message(FATAL_ERROR "The file ${android_jar} does not exist")
  endif()
endfunction()
