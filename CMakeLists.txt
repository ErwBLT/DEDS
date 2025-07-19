cmake_minimum_required(VERSION 3.30)
project(DEDS)

set(CMAKE_CXX_STANDARD 20)

add_executable(DEDS main.cpp
        utils/Web3Client.cpp
        utils/Web3Client.h
        utils/Contract.cpp
        utils/Contract.h
        exchanges/ExchangeBase.cpp
        exchanges/ExchangeBase.h
        exchanges/Pool.h
        exchanges/Token.h
        exchanges/adapters/Uniswap/UniswapV2.cpp
        exchanges/adapters/Uniswap/UniswapV2.h
        utils/Utils.cpp
        utils/Utils.h
        exchanges/adapters/Uniswap/UniswapV3.cpp
        exchanges/adapters/Uniswap/UniswapV3.h
        exchanges/Token.cpp
)

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(cryptopp CONFIG REQUIRED)
find_library(GMP_LIBRARY gmp REQUIRED)
find_library(GMPXX_LIBRARY gmpxx REQUIRED)
find_path(GMP_INCLUDE_DIR gmp.h REQUIRED)
find_path(GMPXX_INCLUDE_DIR gmpxx.h REQUIRED)

target_link_libraries(DEDS PRIVATE
        cryptopp::cryptopp
        CURL::libcurl
        nlohmann_json::nlohmann_json
        ${GMPXX_LIBRARY}
        ${GMP_LIBRARY}
)

target_include_directories(DEDS PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${GMP_INCLUDE_DIR}
        ${GMPXX_INCLUDE_DIR}
)
