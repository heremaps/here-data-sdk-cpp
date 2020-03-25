#!/bin/bash -e
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

# This is script for verifying commit message by SDK requirements.
# Best git and linux practices are included.
# Requirements can be find in scripts/misc/commit_message_recom.txt

# Saving whole commit message into file for reading below
set -x
git branch
branch_name="$(git branch | grep \* | cut -d ' ' -f2)" || branch_name="master"
if [ "$branch_name" = "master" ]; then
    echo "`git log --pretty=format:'%B' -1`" >> commit.log
else
    echo "`git log --pretty=format:'%B' -2 | sed '1d' | sed '1d' `" >> commit.log
fi
set +x

# Counting number of lines in file
num_lines=`wc -l commit.log| cut -d'c' -f1` 

# Loop for every line in file
for (( line=1; line<=${num_lines}; line++ ))
do
    current_line_len=`cat commit.log| sed -n ${line}p |wc -m | sed 's/\ \ \ \ \ \ /''/g' `
    if [ $line -eq 1 ] && [ ${current_line_len} -gt 50 ] ; then
        echo "ERROR: Title is ${current_line_len} length, so it's too long for title. It must be less than 50 chars "
        exit 1
    fi
    if [ $line -eq 2 ] && [ ${current_line_len} -ne 1 ] ; then
        echo "ERROR: Second line in Commit Message is not zero length"
        exit 1
    fi
    if [ $line -eq 3 ] && [ ${current_line_len} -lt 1 ] && [ -n $(cat commit.log| sed -n ${line}p | grep 'See also: ') ] && [ -n $(cat commit.log| sed -n ${line}p | grep 'Relates-To: ') ] && [ -n $(echo ${current_line_len}| grep 'Resolves: ') ] ; then
        echo "ERROR: No details added to commit message besides title starting from 3rd line."
        exit 1
    fi
    if [ ${current_line_len} -gt 72 ] ; then
        echo "ERROR: ${current_line_len} chars in ${line}-th line is too long. This line must be less than 72 chars"
        exit 1
    fi
    echo " ${line}-th line is ${current_line_len} chars length"
done

relates_to=$(cat commit.log | grep 'Relates-To: ') || true
resolves=$(cat commit.log | grep 'Resolves: ') || true
see=$(cat commit.log | grep 'See also: ') || true

echo "Reference like:  ${relates_to} ${resolves} ${see}  was found in commit message."

if [[ -n ${relates_to} || -n ${resolves} || -n ${see} ]] ; then
    echo "Commit message contains Jira link."
else
    echo "ERROR: Commit message does not contain ticket reference like Relates-To: or Resolves: or See also:  "
    echo "Please read following rules:"
    cat scripts/misc/commit_message_recom.txt
    exit 1
fi
