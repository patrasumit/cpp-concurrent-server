cmake_minimum_required(VERSION 3.16)

project(CppConcurrentServer
    VERSION 0.1.0
    LANGUAGES CXX
)

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)

FetchContent_MakeAvailable(googletest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(server_core
    src/http/HttpRequest.cpp
    src/http/HttpResponse.cpp
    src/http/HttpUtils.cpp
    src/http/Router.cpp

    src/networking/Connection.cpp

    src/threadpool/ThreadPool.cpp
)

target_include_directories(server_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

add_executable(cpp_server
    src/main.cpp
)

target_link_libraries(cpp_server
    PRIVATE
        server_core
)

enable_testing()

add_executable(http_request_tests
    tests/HttpRequestTests.cpp
    tests/HttpResponseTests.cpp
    tests/RouterTests.cpp
    tests/ConnectionTests.cpp
)

target_link_libraries(http_request_tests
    PRIVATE
        server_core
        GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(http_request_tests)
