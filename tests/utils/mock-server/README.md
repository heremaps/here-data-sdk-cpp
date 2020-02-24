# MockServer

## What is MockServer

Read more at <https://mock-server.com/>

## Mocking Responses and Setting Expectations

Follow the [documentation](https://mock-server.com/mock_server/creating_expectations.html) for the MockServer.

**Note:** MockServer will play expectations in the exact order they are added. For example, if an expectation `A` is added with `Times.exactly(3)` then expectation `B` is added with `Times.exactly(2)` with the same request matcher they will be applied in the following order `A, A, A, B, B`.

## Getting Started

You can start this server very easily

```bash
cd tests/utils/mock-server
npm install
node server.js
```
