# Local OLP server

This folder contains sources of a server that mimics the OLP services.

Supported operations:

* Lookup API
* Retrieve catalog configuration
* Retrieve catalog latest version
* Retrieve layers versions
* Retrieve layer metadata (partitions)
* Retrieve data from a blob service

Requests are always valid (no validation performed).
Blob service returns generated text data (400-500 kb. size)

## How to run a server

To run a server execute the script:

```shell
node server.js
```

The server implemented as a proxy between the client and the OLP. Clients can connect to it directly; no authorization required.
Note: server supports HTTP protocol only.