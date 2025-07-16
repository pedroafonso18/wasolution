cmake_minimum_required(VERSION 3.10)

# Add this line to set the policy
cmake_policy(SET CMP0074 NEW)

project(wasolution LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform specific settings
if(WIN32)
    # Set OpenSSL paths for Windows
    set(OPENSSL_ROOT_DIR "C:/msys64/mingw64")
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
    set(OPENSSL_LIBRARIES
        "${OPENSSL_ROOT_DIR}/lib/libssl.dll.a"
        "${OPENSSL_ROOT_DIR}/lib/libcrypto.dll.a"
    )

    # Set ZLIB paths for Windows
    set(ZLIB_ROOT "C:/msys64/mingw64")
    set(ZLIB_INCLUDE_DIR "${ZLIB_ROOT}/include")
    set(ZLIB_LIBRARY "${ZLIB_ROOT}/lib/libz.dll.a")

    # Windows link directories
    link_directories(
        "${OPENSSL_ROOT_DIR}/lib"
        "${ZLIB_ROOT}/lib"
    )
else()
    # For Linux/Docker, use system libraries
    find_package(OpenSSL REQUIRED)
    # ZLIB will be found normally
endif()

find_package(Boost REQUIRED COMPONENTS system thread)
find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)

set(SOURCES
    src/main.cpp
    src/api/evolution.cpp
    src/api/wuzapi.cpp
    src/api/api_constants.cpp
    src/config/config.cpp
    src/database/database.cpp
    src/handler/handler.cpp
    src/logger/logger.cpp
    src/cloud/cloud_api.cpp
    src/cloud/cloud_api.h
    src/cloud/cloud_constants.h
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
)

add_executable(wasolution ${SOURCES})

# Platform specific libraries
if(WIN32)
    target_link_libraries(wasolution PRIVATE
        ${Boost_LIBRARIES}
        ws2_32
        mswsock
        CURL::libcurl
        ${OPENSSL_LIBRARIES}
        ${ZLIB_LIBRARY}
        pqxx
        pq
    )
else()
    # Linux-specific libraries
    target_link_libraries(wasolution PRIVATE
        ${Boost_LIBRARIES}
        CURL::libcurl
        OpenSSL::SSL
        OpenSSL::Crypto
        ZLIB::ZLIB
        pqxx
        pq
    )
endif()

if(MSVC)
    target_compile_options(wasolution PRIVATE /W4)
else()
    target_compile_options(wasolution PRIVATE -Wall -Wextra -Wpedantic)
endif()