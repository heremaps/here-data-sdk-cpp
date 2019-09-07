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

const sleep = (milliseconds) => {
    return new Promise(resolve => setTimeout(resolve, milliseconds))
}

const requestHandler = async (request, response) => {

    request.on('error', (err) => {
        console.error(err);
        response.writeHead(400, {}).end();
    });
    response.on('error', (err) => {
        console.error(err);
    });

    const { headers, method, url } = request;
    if (url == '/long_delay' && method == 'GET') {
        await sleep(3000)
        response.writeHead(200, {})
        response.end('Lond delay GET handler')
        return
    }

    if (url == '/get_request' && method == 'GET') {
        response.writeHead(200, {})
        response.end('GET handler')
        return
    }

    if (url == '/echo' && method == 'POST') {
        response.writeHead(200, {})
        request.pipe(response);
        return
    }

    if (url == '/error_404' && method == 'GET') {
        response.writeHead(404, {})
        response.end('Not Found')
        return
    }

    if (url == "http://platform.here.com/" && method == 'GET') {
        const expectedAuthString = 'Basic ' + Buffer.from("test_user:test_password").toString('base64')
        const proxyString = headers['proxy-authorization']
        if (proxyString && proxyString == expectedAuthString) {
            response.writeHead(200, {})
            response.end("Success")
            return
        }
    }

    console.log(request)
    response.writeHead(404, {})
    response.end('Not Found')
}

const server = http.createServer(requestHandler)

server.listen(port, (err) => {
    if (err) {
        return console.log('something bad happened', err)
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