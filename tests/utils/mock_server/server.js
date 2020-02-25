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

const http = require('http')

const port = 3000

const requestHandler = async (request, response) => {
    const { headers, method, url } = request;

    if (url == '/get_request' && method == 'GET') {
        response.writeHead(200, {})
        response.end('GET handler')
    } else {
        console.log(request)
        response.writeHead(404, {})
        response.end('Not Found')
    }
}

const server = http.createServer(requestHandler)

server.listen(port, (err) => {
    if (err) {
        return console.log('Error occurred: ', err)
    }

    console.log(`server is listening on ${port}`)
})

// Handle termination by Travis
process.on('SIGTERM', function () {
    server.close(function () {
        console.log(`Graceful shutdown`)
        process.exit(0);
    });
});
