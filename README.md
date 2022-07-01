# HERE Data SDK for C++

HERE Data SDK for C++ is a C++ client for the <a href="https://platform.here.com" target="_blank">HERE platform</a>.

## Health check

### Build and test

| Linux GCC | Linux CLang | MacOS |
| :-------- | :---------- | :---- |
| [![Linux gcc build status][1]][2] | [![Linux Clang build status][3]][4] | [![MacOS build status][5]][4] |

| iOS | Windows | Android |
| :-- | :------ | :------ |
| [![iOS build status][6]][4] | [![Windows build status][7]][4] | [![Android build status][8]][4] |

[1]: https://github.com/heremaps/here-data-sdk-cpp/workflows/CI/badge.svg?branch=master
[2]: https://github.com/heremaps/here-data-sdk-cpp/actions?query=workflow%3ACI+branch%3Amaster
[3]: https://dev.azure.com/heremaps/here-data-sdk/_apis/build/status/heremaps.here-data-sdk-cpp?branchName=master&jobName=Linux_build_clang
[4]: https://dev.azure.com/heremaps/here-data-sdk/_build/latest?definitionId=3&branchName=master
[5]: https://dev.azure.com/heremaps/here-data-sdk/_apis/build/status/heremaps.here-data-sdk-cpp?branchName=master&jobName=MacOS_build
[6]: https://dev.azure.com/heremaps/here-data-sdk/_apis/build/status/heremaps.here-data-sdk-cpp?branchName=master&jobName=iOS_build
[7]: https://dev.azure.com/heremaps/here-data-sdk/_apis/build/status/heremaps.here-data-sdk-cpp?branchName=master&jobName=Windows_build
[8]: https://dev.azure.com/heremaps/here-data-sdk/_apis/build/status/heremaps.here-data-sdk-cpp?branchName=master&jobName=Android_build

### Test coverage

| Platform | Status                                                                                                                                                                                              |
| :------- | :-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Linux    | <a href="https://codecov.io/gh/heremaps/here-data-sdk-cpp/" target="_blank"><img src="https://codecov.io/gh/heremaps/here-data-sdk-cpp/branch/master/graph/badge.svg" alt="Linux code coverage"/></a> |

## Backward compatibility

We try to develop and maintain our API in a way that preserves its compatibility with the existing applications. Changes in Data SDK for C++ are greatly influenced by the Data API development. Data API introduces breaking changes 6 months in advance. Therefore, you may need to migrate to a new version of Data SDK for C++ every half a year.

For more information on Data API, see its <a href="https://developer.here.com/documentation/data-api/data_dev_guide/index.html" target="_blank">Developer Guide</a> and <a href="https://developer.here.com/documentation/data-api/api-reference.html" target="_blank">API Reference</a>.

When new API is introduced in Data SDK for C++, the old one is not deleted straight away. The standard API deprecation time is 6 months. It gives you time to switch to new code. However, we do not provide ABI backward compatibility.

All of the deprecated methods, functions, and parameters are documented in the Data SDK for C++ <a href="https://developer.here.com/documentation/sdk-cpp/api_reference/index.html" target="_blank">API Reference</a> and <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/CHANGELOG.md" target="_blank">changelog</a>.

For more information on Data SDK for C++, see our <a href="https://developer.here.com/documentation/sdk-cpp/dev_guide/index.html" target="blank">Developer Guide</a>.

## Supported platforms

The table below lists the platforms on which the Data SDK has been tested.

| Platform                   | Minimum requirement     |
| :------------------------- | :---------------------- |
| Ubuntu Linux               | GCC 7.5 and Clang 7.0   |
| Embedded Linux (32 bit)    | GCC 7.4 armhf           |
| Windows                    | MSVC++ 2017             |
| macOS                      | Apple Clang 11.0.0      |
| iOS                        | Xcode 11.1, Swift 5.0   |
| Android                    | API level 21            |

<h6 id="dependencies"></h6>

## Dependencies

The table below lists the dependencies of the Data SDK.

| Library              | Minimum version |
| :------------------- | :-------------- |
| libcurl              | 7.52.0          |
| OpenSSL              | 1.1.1           |
| Boost (headers only) | 1.69.0          |
| LevelDB              | 1.21            |
| Snappy               | 1.1.7           |
| RapidJSON            | latest          |

To install the dependencies on Linux, run the following command:

```bash
sudo apt-get update && sudo apt-get --yes install git g++ make cmake libssl-dev libcurl4-openssl-dev libboost-all-dev
```

> #### Note
> Please note that on some Linux distribution, the default libcurl version can be lower than the required v7.52.0. In that case, you need to install the required libcurl version from a different PPA.

## Use the SDK

To learn how to use the Data SDK, see the <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/docs/GettingStartedGuide.md" target="_blank">Getting Started Guide</a>, <a href="https://developer.here.com/documentation/sdk-cpp/dev_guide/index.html" target="blank">Developer Guide</a>, and <a href="https://developer.here.com/documentation/sdk-cpp/api_reference/index.html" target="_blank">API Reference</a>.

## Contribution

For details, see <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/CONTRIBUTING.md" target="_blank">HERE Data SDK C++ Contributors Guide</a>.

## LICENSE

Copyright (C) 2019–2022 HERE Europe B.V.

For license details, see the <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/LICENSE" target="_blank">LICENSE</a> file in the root of this project.

> #### Note
> This project has Open Source Software dependencies that are downloaded and installed upon execution of the abovementioned installation commands. For further details, see the CMake configuration files included in the <a href="https://github.com/heremaps/here-data-sdk-cpp/tree/master/external" target="_blank">external</a> directory.
