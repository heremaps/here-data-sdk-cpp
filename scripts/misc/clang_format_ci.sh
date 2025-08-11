#!/bin/bash
#
# Copyright (C) 2025 HERE Europe B.V.
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

# Important 2 lines
set +e
set -x

# This script gets the changed files in the pull request, and runs
# clang-format tool to verify them

# Get the current branch name
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)

if [[ $CURRENT_BRANCH == "master" ]]; then
  printf "Currently in master branch, skipping clang-format run.\n"
  exit 0
else
  printf "Currently in %s branch. Running clang-foramt.\n" "$CURRENT_BRANCH"
fi

git branch --all
git fetch origin master
git branch --all
# Get affected files and filter source files
FILES=$(git diff-tree --no-commit-id --name-only --diff-filter=d -r origin/master "$CURRENT_BRANCH" \
        | grep '\.c\|\.cpp\|\.cxx\|\.h\|\.hpp\|\.hxx\|\.mm')

if [ -z "$FILES" ]; then
  printf "No affected files, exiting.\n"
  exit 0
else
  printf "Affected files:\n %s\n" "$FILES"
fi

printf "\n\n### Running clang-format - START ### \n\n"

clang-format-6.0 -i ${FILES}
RESULT=$?

printf "\n\n### Running clang-format - DONE ### \n\n"

if [ "$RESULT" -ne "0" ]; then
  printf "\n\nClang-format failed!\n\n"
  exit ${RESULT}
fi

CLANG_FORMAT_FILE=${CLANG_FORMAT_FILE:-clang-format.diff}
git diff > ${CLANG_FORMAT_FILE}
if [ -s ${CLANG_FORMAT_FILE} ] ; then
  echo "Unformatted files are detected. "
  echo "You may apply provided patch. Download from the workflow summary and unpack to root of repository."
  echo "Then run: git apply $CLANG_FORMAT_FILE"
  # No error here, otherwise github is skipping further steps so artifacts are not stored
fi
