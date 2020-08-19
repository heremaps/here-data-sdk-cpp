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

var crypto = require('crypto')

function computeHash(data) {
    return crypto.createHash('sha1').update(data).digest('hex').toUpperCase();
}

function generateGetBlobApiResponse(request) {
    const layer = request[1]
    const dataHandle = request[2] // ignored
    
    // Generate a blob 400-500 Kb
    var response = ""
    var length = (Math.floor(Math.random() * 100) + 400) * 1024
    var characters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    var charactersLength = characters.length;
    for ( var i = 0; i < length; i++ ) {
        response += characters.charAt(Math.floor(Math.random() * charactersLength));
    }
    
    //console.log(computeHash(response))
    return computeHash(response) + response;
}

const methods = [
{
    regex: /layers\/(.+)\/data\/(.+)$/,
    handler: generateGetBlobApiResponse
}
]

function blob_handler(pathname, query) {
    for (method of methods) {
        const match = pathname.match(method.regex)
        if (match) {
            const response = method.handler(match, query)
            return { status: 200, text: response, headers : {"Content-Type": "application/x-protobuf"} }
        }
    }
    console.log("Not handled", pathname)
    return { status: 404, text: "Not Found" }
}

exports.handler = blob_handler