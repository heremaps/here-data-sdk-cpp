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

function loremIpsum() {
    return "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."
}

function generateCatalogApiResponse(request) {
    const HRN = request[1]
    return {
        id: "Some unique Id",
        hrn: HRN,
        name: "Generated Catalog",
        description: loremIpsum(),
        layers: [
        {
            id: "versioned_test_layer",
            description: loremIpsum(),
            schema: {
                hrn: "hrn:here:schema:::com:here-tile-schema_v1:1.0.0"
            },
            layerType: "versioned"
        },
        {
            id: "volatile_test_layer",
            description: loremIpsum(),
            schema: {
                hrn: "hrn:here:schema:::com:here-tile-schema_v1:1.0.0"
            },
            layerType: "volatile"
        }
        ],
        marketplaceReady: false,
        version: 11
    }
}

const methods = [
{
    regex: /catalogs\/(.+)$/,
    handler: generateCatalogApiResponse
}
]

function config_handler(pathname, query) {
    for (method of methods) {
        const match = pathname.match(method.regex)
        if (match) {
            return { status: 200, text: JSON.stringify(method.handler(match)), headers : {"Content-Type": "application/json"} }
        }
    }
    console.log("Not handled", pathname)
    return { status: 404, text: "Not Found" }
}

exports.handler = config_handler