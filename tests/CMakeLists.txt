cmake_minimum_required(VERSION 3.16)
project(ESPressioSocketsTests LANGUAGES CXX)

enable_testing()

if(NOT ESPRESSIO_COMMAND_INCLUDE_DIR)
    message(FATAL_ERROR "ESPRESSIO_COMMAND_INCLUDE_DIR is required")
endif()

add_executable(test_socket_command test_socket_command.cpp)
target_compile_features(test_socket_command PRIVATE cxx_std_17)
target_compile_options(test_socket_command PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_include_directories(test_socket_command PRIVATE ../src ${ESPRESSIO_COMMAND_INCLUDE_DIR})
add_test(NAME SocketCommand COMMAND test_socket_command)

if(ESPRESSIO_TIMING_INCLUDE_DIR AND ESPRESSIO_UNITS_INCLUDE_DIR AND ESPRESSIO_OBSERVABLE_INCLUDE_DIR)
    add_executable(test_clock_sync_protocol test_clock_sync_protocol.cpp)
    target_compile_features(test_clock_sync_protocol PRIVATE cxx_std_17)
    target_compile_options(test_clock_sync_protocol PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_include_directories(test_clock_sync_protocol PRIVATE
        ../src
        ../src/timing
        ${ESPRESSIO_TIMING_INCLUDE_DIR}
        ${ESPRESSIO_UNITS_INCLUDE_DIR}
        ${ESPRESSIO_OBSERVABLE_INCLUDE_DIR}
    )
    add_test(NAME ClockSynchronizationProtocol COMMAND test_clock_sync_protocol)
endif()
