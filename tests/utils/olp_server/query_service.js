/*
 * Copyright (C) 2019 HERE Europe B.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 * License-Filename: LICENSE
 */

function randomString(length) {
    var result           = '';
    var characters       = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    var charactersLength = characters.length;
    for ( var i = 0; i < length; i++ ) {
        result += characters.charAt(Math.floor(Math.random() * charactersLength));
    }
    return result;
}

function generatePartitions(requested_partitions, layer, version) {
    var partitions = []
    for (var i = 0; i < requested_partitions.length; i++) {
        partitions.push({
            version: Math.floor(Math.random() * version),
            partition: requested_partitions[i],
            layer: layer,
            dataHandle: randomString(35)
        })
    }
    return partitions
}

function generateGetPartitionsApiResponse(request, query) {
    const layer = request[1]    

    var partitions = null
    if (query.partition instanceof Array) 
    {
        partitions = query.partition
    }
    else
    {
        partitions = [query.partition]
    }

    return {
        partitions: generatePartitions(partitions, layer, query.version)
    }
}

const methods = [
{
    regex: /layers\/(.+)\/partitions$/,
    handler: generateGetPartitionsApiResponse
}
]

function query_handler(pathname, query) {
    for (method of methods) {
        const match = pathname.match(method.regex)
        if (match) {
            const response = method.handler(match, query)
            return { status: 200, text: JSON.stringify(response), headers : {"Content-Type": "application/json"} }
        }
    }
    console.log("Not handled", pathname)
    return { status: 404, text: "Not Found" }
}

exports.handler = query_handler