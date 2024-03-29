
cmake_minimum_required(VERSION 3.10.2)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
   set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
   set(CMAKE_XCODE_ATTRIBUTE_CC           "${CMAKE_SOURCE_DIR}/ci/ccache_clang")
   set(CMAKE_XCODE_ATTRIBUTE_CXX          "${CMAKE_SOURCE_DIR}/ci/ccache_clang++")
   set(CMAKE_XCODE_ATTRIBUTE_LD           "${CMAKE_SOURCE_DIR}/ci/ccache_clang")
   set(CMAKE_XCODE_ATTRIBUTE_LDPLUSPLUS   "${CMAKE_SOURCE_DIR}/ci/ccache_clang++")
endif()

option(HUNTER_RUN_UPLOAD "Upload Hunter packages to binary cache server" OFF)

set(
   HUNTER_CACHE_SERVERS
   "https://github.com/koinos/hunter-cache"
   CACHE
   STRING
   "Default cache server"
)

set(
   HUNTER_PASSWORDS_PATH
   "${CMAKE_CURRENT_LIST_DIR}/cmake/Hunter/passwords.cmake"
   CACHE
   FILEPATH
   "Hunter passwords"
)

include("cmake/HunterGate.cmake")

HunterGate(
   URL "https://github.com/cpp-pm/hunter/archive/v0.23.313.tar.gz"
   SHA1 "6c369e5aab2749a616917df3b23511535f413604"
   LOCAL
)

project(koinos_tools VERSION 0.1.0 LANGUAGES CXX C)

option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." OFF)

# This is to force color output when using ccache with Unix Makefiles
if(${FORCE_COLORED_OUTPUT})
    if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" )
       add_compile_options (-fdiagnostics-color=always)
    elseif( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang" )
       add_compile_options (-fcolor-diagnostics)
    endif ()
endif ()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(Boost_NO_BOOST_CMAKE ON)

if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Werror -Wno-unknown-pragmas)
endif()

if(${CMAKE_GENERATOR} MATCHES "Xcode")
  set(CMAKE_XCODE_GENERATE_SCHEME YES)
endif()

hunter_add_package(Boost COMPONENTS system log thread date_time filesystem chrono test program_options)
hunter_add_package(ethash)
hunter_add_package(libsecp256k1-vrf)
hunter_add_package(nlohmann_json)
hunter_add_package(OpenSSL)
hunter_add_package(rabbitmq-c)
hunter_add_package(rocksdb)
hunter_add_package(yaml-cpp)
hunter_add_package(Protobuf)

hunter_add_package(koinos_log)
hunter_add_package(koinos_util)
hunter_add_package(koinos_proto)
hunter_add_package(koinos_exception)
hunter_add_package(koinos_crypto)
hunter_add_package(koinos_mq)

find_package(Boost CONFIG REQUIRED COMPONENTS system log log_setup thread date_time filesystem chrono program_options)
find_package(ethash CONFIG REQUIRED)
find_package(libsecp256k1-vrf CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(rabbitmq-c CONFIG REQUIRED)
find_package(RocksDB CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

find_package(koinos_crypto CONFIG REQUIRED)
find_package(koinos_exception CONFIG REQUIRED)
find_package(koinos_log CONFIG REQUIRED)
find_package(koinos_mq CONFIG REQUIRED)
find_package(koinos_proto CONFIG REQUIRED)
find_package(koinos_util CONFIG REQUIRED)

add_subdirectory(programs)
