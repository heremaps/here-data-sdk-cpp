#!/bin/bash
#
# Copyright (C) 2020-2021 HERE Europe B.V.
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
# cpplint tool to verify them

# Get the current branch name
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)

if [[ $CURRENT_BRANCH == "master" ]]; then
  printf "Currently in master branch, skipping cpplint.\n"
  exit 0
else
  printf "Currently in %s branch. Running cpplint.\n" "$CURRENT_BRANCH"
fi

git branch --all
git fetch origin master
git branch --all
# Get affected files and filter source files
FILES=$(git diff-tree --no-commit-id --name-only -r origin/master "$CURRENT_BRANCH" \
        | grep '\.c\|\.cpp\|\.cxx\|\.h\|\.hpp\|\.hxx')

if [ -z "$FILES" ]; then
  printf "No affected files, exiting.\n"
  exit 0
else
  printf "Affected files:\n %s\n" "$FILES"
fi

# Install cpplint tool using pip
printf "Installing cpplint using pip\n\n"

pip3 install --user cpplint==1.6.1
export PATH=$PATH:~/.local/bin/
printf "\n\nRunning cpplint\n\n"

# Run the cpplint tool and collect all warnings
cpplint $FILES 2> warnings.txt
RESULT=$?

if [ "$RESULT" -ne "0" ]; then
  printf "\n\nNew warnings.txt found, please check them:\n\n"
  cat warnings.txt
  # temporary skipped
  # exit $RESULT
  echo "We might exit with $RESULT now"
fi

printf "\n\nComplete cpplint\n\n"

cat warnings.txt

printf "\n\nRunning cpplint again and check no new warnings found\n\n"

CPPLINT_BLOCKING_FILTER="\
     -runtime/references,\
     -runtime/string,\
     -runtime/int,\
     -runtime/explicit,\
     -runtime/arrays,\
     -runtime/threadsafe_fn,\
     -build/include_subdir,\
     -build/include_order,\
     -build/include_what_you_use,\
     -build/c++11,\
     -readability/multiline_string,\
     -readability/casting,\
     -readability/todo,\
     -readability/alt_tokens,\
     -readability/braces,\
     -readability/check,\
     -readability/multiline_comment,\
     -whitespace/comma,\
     -whitespace/parens,\
     -whitespace/braces,\
     -whitespace/line_length,\
     -whitespace/comments"

cpplint --filter="$CPPLINT_BLOCKING_FILTER" $FILES 2> errors.txt
RESULT=$?

if [ "$RESULT" -ne "0" ]; then
  printf "\n\nNew errors found, please check them:\n\n"
  cat errors.txt
  exit $RESULT
fi

# Check for public headers modified, must have no new warnings:
# dataservice-read/include
# dataservice-write/include
CRITICAL_PATHS=$(echo "$FILES" | grep "read/include\|write/include")

if [ ! -z "$CRITICAL_PATHS" ]; then
  printf "\n\nPublic headers modified, running full check on them\n\n"
  CPPLINT_IGNORED_WARNINGS="--filter=-build/include_order"
  cpplint $CPPLINT_IGNORED_WARNINGS $CRITICAL_PATHS 2> errors.txt
  RESULT=$?
  printf "\n\nComplete cpplint for protected paths\n\n"
  cat errors.txt
  exit $RESULT
else
  # Despite the error found exit without errors.
  # Remove this exit to make the script blocking on CI.
  exit 0
fi
