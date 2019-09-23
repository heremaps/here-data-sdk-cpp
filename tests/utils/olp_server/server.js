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
const URL = require('url')
const services = require('./urls.js')

const lookup_service_handler = require('./lookup_service.js')
const config_service_handler = require('./config_service.js')
const metadata_service_handler = require('./metadata_service.js')
const query_service_handler = require('./query_service.js')
const blob_service_handler = require('./blob_service.js')

const port = 3000

const sleep = (milliseconds) => {
    return new Promise(resolve => setTimeout(resolve, milliseconds))
}

const handlers = {}
handlers[services.lookup] = lookup_service_handler.handler
handlers[services.config] = config_service_handler.handler
handlers[services.metadata] = metadata_service_handler.handler
handlers[services.query] = query_service_handler.handler
handlers[services.blob] = blob_service_handler.handler

const requestHandler = async (request, response) => {

    request.on('error', (err) => {
        console.error(err);
        response.writeHead(400, {}).end();
    });
    response.on('error', (err) => {
        console.error(err);
    });

    const { headers, method, url } = request;
    
    // Currently we support only read operations
    if (method != 'GET') {
        response.writeHead(404, {})
        response.end('Not Found')
        return
    }
    
    // For debug purpose
    console.log(url)
    
    const { host, query, pathname } = URL.parse(url, true)
    
    const handler = handlers[host]
    if (handler) {
        const result = handler(pathname, query)
        response.writeHead(result.status, result.headers)
        response.end(result.text)
        return
    }
    
    response.writeHead(400, {})
    response.end('Not implemented')
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
