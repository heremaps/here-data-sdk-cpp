# Copyright (C) 2019-2020 HERE Europe B.V.
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

# Returns a shell command that can be executed to recursively remove a directory correctly
# handling any symlinks inside, usable on *nix and Windows.
# Usage:
# get_remove_directory_ignore_symlinks_command(command directory)
# command: name of a variable that receives the command
# directory: the directory to remove
function(get_remove_directory_ignore_symlinks_command command directory)

    if (CMAKE_HOST_WIN32)
        set(script_file_name "${CMAKE_BINARY_DIR}/scripts/remove-directory-ignore-symlinks.bat")
        file(WRITE ${script_file_name} "\
@echo off
set target_dir=%1
set target_dir=%target_dir:/=\\%
if not exist %target_dir% goto done
rmdir /q /s %target_dir%
:done
exit /b 0")

        set(${command} cmd.exe /C ${script_file_name} ${directory} PARENT_SCOPE)
    else()
        set(${command} rm -rf ${directory} PARENT_SCOPE)
    endif()

endfunction()

# Creates a custom command to remove a directory, ignoring symlinks
# Usage:
# remove_directory_ignore_symlinks_custom_command(target type directory)
# target: the name of the target to apply the custom command to
# type: PRE_BUILD, PRE_LINK, or POST_BUILD
# directory: the directory to remove
function(remove_directory_ignore_symlinks_custom_command target type directory)
    get_remove_directory_ignore_symlinks_command(command ${directory})
    add_custom_command(TARGET ${target} ${type} COMMAND ${command})
endfunction()

# Removes a directory, ignoring symlinks
# Usage:
# remove_directory_ignore_symlinks(directory)
# directory: the directory to remove
function(remove_directory_ignore_symlinks directory)
    get_remove_directory_ignore_symlinks_command(command ${directory})
    execute_process(COMMAND ${command})
endfunction()

# Returns a shell command that can be executed to generate a symlink to a file
# or a directory, usable on *nix and Windows.
# Usage:
# get_symlink_file_command(command source destination)
# command: name of a variable that receives the command
# type: FILE or DIR
# source: the source path for the symlink
# destination: the destination path to create the symlink
function(get_symlink_command command type source destination)

    if(CMAKE_HOST_WIN32)

        if("${type}" STREQUAL "DIR")
            set(mklink_option "d")
        elseif("${type}" STREQUAL "FILE")
            set(mklink_option "f")
        else()
            message(FATAL_ERROR "Unsupported link type: '${type}'")
        endif()

        set(script_file_name "${CMAKE_BINARY_DIR}/scripts/get-symlink-command.bat")
        file(WRITE ${script_file_name} "\
@echo off
set destination=%2
set destination=%destination:/=\\%
set source=%3
set source=%source:/=\\%
set options=
if \"%1\"==\"d\" (
  set options=/J
  if exist %destination% (
    rmdir /q /s %destination%
  )
) else (
  if exist %destination% (
    del /q %destination%
  )
)
mklink %options% %destination% %source% > NUL
exit /b 0")

        set(${command} cmd.exe /C ${script_file_name} ${mklink_option} ${destination} ${source}
            PARENT_SCOPE)

    else()
        set(${command} ${CMAKE_COMMAND}
            -E create_symlink ${source} ${destination}
            PARENT_SCOPE)
    endif()

endfunction()

# Creates a custom command to generate a symlink to a file, usable on *nix and Windows.
# Usage:
# create_symlink_file_custom_command(target type source destination)
# target: the name of the target to apply the custom command to
# type: PRE_BUILD, PRE_LINK, or POST_BUILD
# source: the source path for the symlink
# destination: the destination path to create the symlink
function(create_symlink_file_custom_command target type source destination)
    get_symlink_command(command FILE ${source} ${destination})
    add_custom_command(TARGET ${target} ${type} COMMAND ${command})
endfunction()

# Generates a symlink to a file, usable on *nix and Windows.
# Usage:
# create_symlink_file(source destination)
# source: the source path for the symlink
# destination: the destination path to create the symlink
function(create_symlink_file source destination)
    get_symlink_command(command FILE ${source} ${destination})
    execute_process(COMMAND ${command})
endfunction()

# Creates a custom command to generate a symlink to a directory, usable on *nix and Windows.
# Usage:
# create_symlink_directory_custom_command(target type source destination)
# target: the name of the target to apply the custom command to
# type: PRE_BUILD, PRE_LINK, or POST_BUILD
# source: the source path for the symlink
# destination: the destination path to create the symlink
function(create_symlink_directory_custom_command target type source destination)
    get_symlink_command(command DIR ${source} ${destination})
    add_custom_command(TARGET ${target} ${type} COMMAND ${command})
endfunction()

# Generates a symlink to a directory, usable on *nix and Windows.
# Usage:
# create_symlink_directory(source destination)
# source: the source path for the symlink
# destination: the destination path to create the symlink
function(create_symlink_directory source destination)
    get_symlink_command(command DIR ${source} ${destination})
    execute_process(COMMAND ${command})
endfunction()
