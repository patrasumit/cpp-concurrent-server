cmake_minimum_required(VERSION 3.16)

project(CppConcurrentServer
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(cpp_server
    src/main.cpp
    
    src/http/HttpRequest.cpp
    src/http/HttpResponse.cpp
    src/http/HttpUtils.cpp
    src/http/Router.cpp

    src/networking/Connection.cpp

    src/threadpool/ThreadPool.cpp
)

target_include_directories(cpp_server
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)
