# Copyright (c) 2019-2022, The Vulkan Developers.
#
# This file is part of Vulkan.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# You should have received a copy of the MIT License
# along with Vulkan. If not, see <https://opensource.org/licenses/MIT>.

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(vulkan LANGUAGES C ASM)

cmake_policy(SET CMP0079 NEW)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

option(WITH_ROCKSDB "Build with RocksDB support" ON)
option(WITH_LEVELDB "Build with LevelDB support" OFF)
option(WITH_NET_QUEUE "Enable the network send/receive queue, sends/receives data every XXX interval" OFF)
option(BUILD_STATIC "Build a statically linked binary" ON)

if (UNIX)
  find_package(Threads QUIET)
  if (NOT Threads_FOUND)
    message(FATAL_ERROR "Failed to find pthreads dependency!")
  endif()
endif()

find_package(Sodium QUIET)
if (NOT SODIUM_FOUND)
  add_subdirectory(external/libsodium-CMake)
  set(${SODIUM_INCLUDE_DIR} external/libsodium-CMake/src/libsodium/include)
endif()
include_directories(${SODIUM_INCLUDE_DIR})

# ensure that we've set the openssl root and lib directory paths
if (APPLE AND NOT DEFINED ENV{OPENSSL_ROOT_DIR})
  set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl" "/usr/local/ssl/*" "/usr/local/Cellar/openssl/*")
  set(OPENSSL_LIBRARIES "/usr/local/opt/openssl/lib" "/usr/local/ssl/*/lib" "/usr/local/Cellar/openssl/*/lib")
endif()

find_package(OpenSSL QUIET)
if (NOT OPENSSL_FOUND)
  message(FATAL_ERROR "Failed to find OpenSSL dependency!")
else()
  include_directories(${OPENSSL_INCLUDE_DIR})
endif()

if (WITH_LEVELDB)
  find_package(LevelDB QUIET)
  if (NOT LEVELDB_FOUND)
    add_subdirectory(external/leveldb)
    set(${LEVELDB_INCLUDE_DIR} external/leveldb/include)
  endif()
  include_directories(${LEVELDB_INCLUDE_DIR})
  add_definitions(-DUSE_LEVELDB)
elseif (WITH_ROCKSDB)
  find_package(RocksDB QUIET)
  if (NOT ROCKSDB_FOUND)
    add_subdirectory(external/rocksdb)
    set(${ROCKSDB_INCLUDE_DIR} external/rocksdb/include)
  endif()
  include_directories(${ROCKSDB_INCLUDE_DIR})
  add_definitions(-DUSE_ROCKSDB)
else()
  message(FATAL_ERROR "No blockchain storage database backend was specified!")
endif()

if (WITH_NET_QUEUE)
  add_definitions(-DUSE_NET_QUEUE)
endif()

add_subdirectory(external/Collections-C)
include_directories(external/Collections-C/src/include)

add_subdirectory(external/miniupnp/miniupnpc)
include_directories(external/miniupnp/miniupnpc)

include_directories(external/mongoose)
include_directories(src)
include_directories(tests)

add_subdirectory(external)
add_subdirectory(src)
add_subdirectory(tests)
