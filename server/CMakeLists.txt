# Copyright (c) 2019 - present Advanced Micro Devices, Inc. All rights reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

#
# Minimum version of cmake required
#
cmake_minimum_required(VERSION 3.5.0)

message("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&")
message("                             Cmake Server                                 ")
message("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&")

message("")
message("Build Configuration:")
message("-----------BuildType: " ${CMAKE_BUILD_TYPE})
message("------------Compiler: " ${CMAKE_CXX_COMPILER})
message("-------------Version: " ${CMAKE_CXX_COMPILER_VERSION})
message("--------Proj Src Dir: " ${PROJECT_SOURCE_DIR})
message("--------Proj Bld Dir: " ${PROJECT_BINARY_DIR})
message("--------Proj Lib Dir: " ${PROJECT_BINARY_DIR}/lib)
message("--------Proj Exe Dir: " ${PROJECT_BINARY_DIR}/bin)
message("--------RSMI Lib Dir: " ${RSMI_LIB_DIR})
message("--------RSMI Inc Dir: " ${RSMI_INC_DIR})
message("")

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
                   CACHE STRING "Location of RDC client library source code.")
# set(CMAKE_INSTALL_PREFIX "/"
#                                CACHE STRING "Default installation directory.")
# set(CPACK_PACKAGING_INSTALL_PREFIX "/"
#                                      CACHE STRING "Default packaging prefix.")
# 

## Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -m64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2 -std=c++11 ")
# Use this instead of above for 32 bit
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")

if ("${CMAKE_BUILD_TYPE}" STREQUAL Release)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -O0 -DDEBUG")
endif ()

set(SRC_DIR "src")
set(INC_DIR "include/${RDC}")

# TODO delete these if not used
file(GLOB PROTOBUF_GENERATED_INCLUDES "${PROTOB_OUT_DIR}/*.h")
file(GLOB PROTOBUF_GENERATED_SRCS "${PROTOB_OUT_DIR}/*.cc")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include
                     "${PROTOB_OUT_DIR}" "${RSMI_INC_DIR}")

set(SERVER_SRC_LIST "${SRC_DIR}/rdc_rsmi_service.cc")
set(SERVER_SRC_LIST ${SERVER_SRC_LIST} "${SRC_DIR}/rdc_admin_service.cc")
set(SERVER_SRC_LIST ${SERVER_SRC_LIST} "${SRC_DIR}/rdc_server_main.cc")
set(SERVER_SRC_LIST ${SERVER_SRC_LIST} "${PROTOBUF_GENERATED_SRCS}")
message("SERVER_SRC_LIST=${SERVER_SRC_LIST}")

set(SERVER_DAEMON_EXE "rdcd")
set(SERVICE_FILE_NAME "rdc.service")

link_directories(${RSMI_LIB_DIR})

add_executable(${SERVER_DAEMON_EXE} "${SERVER_SRC_LIST}")
# target_include_directories(${SERVER_DAEMON_EXE} PUBLIC ${RSMI_INC_DIR})

target_link_libraries(${SERVER_DAEMON_EXE} pthread rt grpc grpc++
                                     grpc++_reflection dl protobuf rocm_smi64)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SERVER_DAEMON_EXE}
          PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
                                                            DESTINATION usr/sbin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${SERVICE_FILE_NAME}
                                              DESTINATION lib/systemd/system)

message("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&")
message("                    Finished Cmake Server                                 ")
message("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&")