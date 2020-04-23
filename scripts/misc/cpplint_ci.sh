#!/bin/bash +e
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

# This script gets the changed files in the pull request, and runs 
# cpplint tool to verify them

# Get the current branch name
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)

if [[ $CURRENT_BRANCH == "master" ]]; then
  printf "Currently in master branch, skipping cpplint.\n"
  exit 0
else
  printf "Currently in %s branch. Running cpplint.\n" "$CURRENT_BRANCH"
fi

# Get affected files and filter source files
FILES=$(git diff-tree --no-commit-id --name-only -r master "$CURRENT_BRANCH" \
        | grep '\.c\|\.cpp\|\.cxx\|\.h\|\.hpp\|\.hxx')

if [ -z "$FILES" ]; then
  printf "No affected files, exiting.\n"
  exit 0
else
  printf "Affected files:\n %s\n" "$FILES"
fi

# Install cpplint tool using pip
printf "Installing cpplint using pip\n\n"

pip install --user cpplint

printf "\n\nRunning cpplint\n\n"

# Exclude warning about <future> <mutex> includes
CPPLINT_FILTER="--filter=-build/c++11"

# Run the cpplint tool
cpplint $CPPLINT_FILTER $FILES 2> warnigns.txt

printf "\n\nComplete cpplint\n\n"

cat warnigns.txt

# Despite the error found exit without errors.
# Remove this exit to make the script blocking on CI.
exit 0
