# HERE Data SDK for C++

HERE Data SDK for C++ is a C++ client for the <a href="https://platform.here.com" target="_blank">HERE platform</a>.

## Use the SDK

To learn how to install and use the Data SDK, see the <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/docs/GettingStartedGuide.md" target="_blank">Getting Started Guide</a> and <a href="https://www.here.com/docs/bundle/data-sdk-for-cpp-developer-guide/page/README.html" target="blank">Developer Guide</a>.

## Health check

### Build and test pipelines

| Linux GCC | Linux CLang | MacOS |
| :-------- | :---------- | :---- |
| [![Linux gcc build status][1]][1] | [![Linux Clang build status][1]][1] | [![MacOS build status][1]][1] |

| iOS | Windows | Android |
| :-- | :------ | :------ |
| [![iOS build status][1]][1] | [![Windows build status][1]][1] | [![Android build status][1]][1] |

[1]: https://github.com/heremaps/here-data-sdk-cpp/actions/workflows/psv_pipelines.yml/badge.svg

### Full verification testing pipelines

| iOS | Android |
| :-- | :------ |
| [![iOS test status][2]][2] | [![Android test status][2]][2] |

[2]: https://github.com/heremaps/here-data-sdk-cpp/actions/workflows/fv_pipelines.yml/badge.svg 

### Test coverage

| Platform | Status                                                                                                                                                                                              |
| :------- | :-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Linux    | <a href="https://codecov.io/gh/heremaps/here-data-sdk-cpp/" target="_blank"><img src="https://codecov.io/gh/heremaps/here-data-sdk-cpp/branch/master/graph/badge.svg" alt="Linux code coverage"/></a> |

## Backward compatibility

We try to develop and maintain our API in a way that preserves its compatibility with the existing applications. Changes in Data SDK for C++ are greatly influenced by the Data API development. Data API introduces breaking changes 6 months in advance. Therefore, you may need to migrate to a new version of Data SDK for C++ every half a year.

For more information on Data API, see its <a href="https://www.here.com/docs/bundle/data-api-developer-guide/page/README.html" target="_blank">Developer Guide</a> and <a href="https://www.here.com/docs/category/data-api" target="_blank">API Reference</a>.

When new API is introduced in Data SDK for C++, the old one is not deleted straight away. The standard API deprecation time is 6 months. It gives you time to switch to new code. However, we do not provide ABI backward compatibility.

Learn more about deprecated methods, functions, and parameters in the Data SDK for C++ <a href="https://www.here.com/docs/bundle/data-sdk-for-cpp-api-reference/page/index.html" target="_blank">API Reference</a> and <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/CHANGELOG.md" target="_blank">changelog</a>.

For more information on Data SDK for C++, see our <a href="https://www.here.com/docs/bundle/data-sdk-for-cpp-developer-guide/page/README.html" target="blank">Developer Guide</a>.

## Supported platforms

The table below lists the platforms on which the Data SDK has been tested.

| Platform                   | Minimum requirement   |
| :------------------------- |:----------------------|
| Ubuntu Linux               | GCC 7.5 and Clang 7.0 |
| Embedded Linux (32 bit)    | GCC 7.4 armhf         |
| Windows                    | MSVC++ 2017           |
| macOS                      | Apple Clang 11.0.0    |
| iOS                        | Xcode 11.7, Swift 5.0 |
| Android                    | API level 21          |

<h6 id="dependencies"></h6>

## Dependencies

The table below lists the dependencies of the Data SDK.

| Library              | Recommended version |
|:---------------------|:--------------------|
| Libcurl              | 7.52.1              |
| OpenSSL              | 1.1.1w              |
| Boost (headers only) | 1.72.0              |
| LevelDB              | 1.21                |
| Snappy               | 1.1.7               |
| RapidJSON            | latest              |
| Zlib                 | 1.3.1               |

### Linux dependencies

To install the dependencies on Linux, run the following command:

```bash
sudo apt-get update && sudo apt-get --yes install git g++ make cmake libssl-dev libcurl4-openssl-dev libboost-all-dev
```

> #### Note
> Please note that on some Linux distribution, the default libcurl version can be lower than the required v7.52.0. In that case, you need to install the required libcurl version from a different PPA.

## Contribution

For details, see <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/CONTRIBUTING.md" target="_blank">HERE Data SDK C++ Contributors Guide</a>.

## LICENSE

Copyright (C) 2019–2024 HERE Europe B.V.

For license details, see the <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/LICENSE" target="_blank">LICENSE</a> file in the root of this project.

> #### Note
> This project has Open Source Software dependencies that are downloaded and installed upon execution of the abovementioned installation commands. For further details, see the CMake configuration files included in the <a href="https://github.com/heremaps/here-data-sdk-cpp/tree/master/external" target="_blank">external</a> directory.
