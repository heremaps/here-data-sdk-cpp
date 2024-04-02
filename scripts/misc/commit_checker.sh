#!/bin/bash -e
#
# Copyright (C) 2020-2023 HERE Europe B.V.
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

# This is script for verifying commit message by requirements.
# Best git and linux practices are included.
# Requirements can be find in scripts/misc/commit_message_recom.txt

# Saving whole commit message into file for reading below
git log -1 --no-merges --pretty=%B > commit.log
echo "----------------------------------------------"
cat commit.log || true
echo "----------------------------------------------"

# Counting number of lines in file
num_lines=`wc -l commit.log | cut -d'c' -f1`

# Loop for every line in file
for (( line=1; line<=${num_lines}; line++ ))
do
    current_line_len=`cat commit.log | sed -n ${line}p | tr -d '\n\r' | wc -m | sed 's/\ \ \ \ \ \ /''/g' `
    current_line=`cat commit.log | sed -n ${line}p`
    if [ $line -eq 1 ] && [ ${current_line_len} -gt 50 ] ; then
        echo ""
        echo "ERROR: Title is ${current_line_len} length, so it's too long for title. Expect less than or equal to 50 chars !"
        echo ""
        cat commit.log
        echo "----------------------------------------------"
        echo "Please read following rules:"
        cat scripts/misc/commit_message_recom.txt
        exit 6
    fi
    if [ $line -eq 2 ] && [ ${current_line_len} -ne 0 ] ; then
        echo ""
        echo "ERROR: Second line in Commit Message is not zero length !"
        echo ""
        cat commit.log
        echo "----------------------------------------------"
        echo "Please read following rules:"
        cat scripts/misc/commit_message_recom.txt
        exit 5
    fi
    if [ "$line" -eq 3 ] && ( [ "$current_line_len" -le 1 ] || [ -n "$(cat commit.log | sed -n ${line}p | grep 'See also: ')" ] || [ -n "$(cat commit.log | sed -n ${line}p | grep 'Relates-To: ')" ] || [ -n "$(cat commit.log | sed -n ${line}p | grep 'Resolves: ')" ] ) ; then
        echo ""
        echo "ERROR: No details added to commit message besides title and ticket reference!"
        echo ""
        cat commit.log
        echo "----------------------------------------------"
        echo "Please read following rules:"
        cat scripts/misc/commit_message_recom.txt
        exit 4
    fi
    if [[ ${current_line} == *"Signed-off-by"* ]]; then
        echo " ${line}-th line is sign-off line."
        if [ ${current_line_len} -gt 80 ]; then
            echo ""
            echo "ERROR: ${current_line_len} chars in ${line}-th line is too long. Any line length must be less than or equal to 80 chars !"
            echo ""
            cat commit.log
            echo "----------------------------------------------"
            echo "Please read following rules:"
            cat scripts/misc/commit_message_recom.txt
            exit 3
        fi
        echo " ${line}-th line is ${current_line_len} chars length . OK."
        continue
    fi
    if [ ${current_line_len} -gt 72 ] ; then
         echo ""
         echo "ERROR: ${current_line_len} chars in ${line}-th line is too long. Any line length must be less than or equal to 72 chars !"
         echo ""
         cat commit.log
         echo "----------------------------------------------"
         echo "Please read following rules:"
         cat scripts/misc/commit_message_recom.txt
         exit 2
     fi
    echo " ${line}-th line is ${current_line_len} chars length . OK."
done

relates_to=$(cat commit.log | grep 'Relates-To: ') || true
resolves=$(cat commit.log | grep 'Resolves: ') || true
see=$(cat commit.log | grep 'See also: ') || true

# This is verification that we have any of these possible issue references.
if [[ -n "${relates_to}" ]] || [[ -n "${resolves}" ]] || [[ -n "${see}" ]] ; then
    # This verification is needed for correct Jira linking in Gitlab.
    if [[ "$(echo ${relates_to}| cut -d":" -f2)" =~ [a-z] ]] || [[ "$(echo ${resolves}| cut -d":" -f2)" =~ [a-z] ]] || [[ "$(echo ${see}| cut -d":" -f2)" =~ [a-z] ]] ; then
        echo ""
        echo "ERROR: Commit message contains ticket or issue reference in lower case !"
        echo ""
        cat commit.log
        echo "----------------------------------------------"
        echo "Please read following rules:"
        cat scripts/misc/commit_message_recom.txt
        exit 2
    fi
    echo ""
    echo "Commit message contains correct issue reference. OK."
    echo ""
else
    echo ""
    echo "ERROR: Commit message does not contain ticket or issue reference like Relates-To: or Resolves: or See also:  !"
    echo ""
    cat commit.log
    echo "----------------------------------------------"
    echo "Please read following rules:"
    cat scripts/misc/commit_message_recom.txt
    exit 1
fi
