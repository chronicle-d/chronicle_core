cmake_minimum_required(VERSION 3.15...3.29)
project(chronicle LANGUAGES CXX)

# General settings
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Include paths
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/relics/relic-telnet/include
    /usr/include/libssh
)

# Required packages
set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)
find_package(libssh CONFIG REQUIRED)

find_package(mongoc CONFIG REQUIRED)
find_package(bson CONFIG REQUIRED)
find_package(mongocxx REQUIRED)
find_package(bsoncxx REQUIRED)

# Fallback in case CMake can't find the lib directly
find_library(LIBSSH_LIBRARY NAMES ssh PATHS /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib REQUIRED)
message(STATUS "Found LIBSSH_LIBRARY: ${LIBSSH_LIBRARY}")

# Core static library
add_library(chronicle_core STATIC
    src/core/error_handler.cpp
    src/core/device_factory.cpp
)

set_target_properties(chronicle_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(chronicle_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# pybind11 module
pybind11_add_module(chronicle
    src/core/chronicle.cpp
    src/core/error_handler.cpp
    src/core/ssh.cpp
    src/core/config.cpp
    src/core/mongodb.cpp
    src/database_handler.cpp
    src/core/device_factory.cpp
    src/bindings/chronicle.cpp
    src/bindings/devices.cpp
)

target_link_libraries(chronicle PRIVATE ${LIBSSH_LIBRARY} mongo::mongocxx_shared mongo::bsoncxx_shared)

set_target_properties(chronicle PROPERTIES
    OUTPUT_NAME "chronicle"
    SUFFIX ".so"
)

install(TARGETS chronicle DESTINATION .)

# ========= DEVICES SECTION =========
# Macro to declare device modules
macro(chronicle_device vendor name source_file)
  set(target_name ${name})
  add_library(${target_name} SHARED ${source_file})

  target_include_directories(${target_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

  target_link_libraries(${target_name} PRIVATE chronicle_core)

  set_target_properties(${target_name} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/devices/${vendor}
        PREFIX ""
        OUTPUT_NAME ${name}
        SUFFIX ".cld"
    )

  install(TARGETS ${target_name} DESTINATION bin/devices/${vendor})
endmacro()

### CISCO ###
chronicle_device(Cisco CATALYST_8000V src/deviceMaps/Cisco/CATALYST_8000V.cpp)
chronicle_device(Cisco LINUX_TEST src/deviceMaps/Cisco/LINUX_TEST.cpp)

### JUNIPER ###
chronicle_device(Juniper VSRX src/deviceMaps/Juniper/VSRX.cpp)
