########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 3.8)
project(gr-lora_concentrator CXX C)

# Install to PyBIND11 path (conda env)
if(NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "default install prefix")
endif()

set(CMAKE_CXX_STANDARD 17)

# Set the version information
set(VERSION_MAJOR 1)
set(VERSION_API   0)
set(VERSION_ABI   0)
set(VERSION_PATCH 0)

########################################################################
# Compiler-specific settings
########################################################################
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unknown-pragmas")
endif()

########################################################################
# Find gnuradio
########################################################################
find_package(Gnuradio "3.10" REQUIRED COMPONENTS runtime blocks fft filter)

# Find gr-lora_sdr (the base LoRa module we depend on)
find_package(PkgConfig)

########################################################################
# Find paho-mqtt-c (for MQTT blocks)
########################################################################
find_path(PAHO_MQTT_C_INCLUDE_DIR NAMES MQTTClient.h
    PATHS /usr/include /usr/local/include
)
find_library(PAHO_MQTT_C_LIBRARY NAMES paho-mqtt3c paho-mqtt3a
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
)

if(PAHO_MQTT_C_INCLUDE_DIR AND PAHO_MQTT_C_LIBRARY)
    set(PAHO_MQTT_FOUND TRUE)
    message(STATUS "Found Paho MQTT C: ${PAHO_MQTT_C_LIBRARY}")
else()
    set(PAHO_MQTT_FOUND FALSE)
    message(WARNING "Paho MQTT C library not found. MQTT blocks will use stub implementation.")
    message(WARNING "Install with: sudo apt install libpaho-mqtt-dev (Linux) or vcpkg install paho-mqtt (Windows)")
endif()

########################################################################
# Find nlohmann/json (for JSON serialization)
########################################################################
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann/json not found, using bundled header")
endif()

########################################################################
# Find Volk
########################################################################
find_package(Volk REQUIRED)

########################################################################
# Setup python
########################################################################
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 REQUIRED)

########################################################################
# Setup the include and linker paths
########################################################################
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
    ${Gnuradio_INCLUDE_DIRS}
    ${Volk_INCLUDE_DIRS}
)

if(PAHO_MQTT_FOUND)
    include_directories(${PAHO_MQTT_C_INCLUDE_DIR})
endif()

########################################################################
# Create uninstall target
########################################################################
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
@ONLY)

add_custom_target(uninstall
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)

########################################################################
# Add subdirectories
########################################################################
add_subdirectory(include/gnuradio/lora_concentrator)
add_subdirectory(lib)
add_subdirectory(python/lora_concentrator)
add_subdirectory(grc)
add_subdirectory(apps)
