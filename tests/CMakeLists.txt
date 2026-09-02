cmake_minimum_required(VERSION 3.16)
project(ESPressioSocketsTests LANGUAGES CXX)

enable_testing()

if(NOT ESPRESSIO_SYSTEM_INCLUDE_DIR)
    message(FATAL_ERROR "ESPRESSIO_SYSTEM_INCLUDE_DIR is required for portable Sockets tests")
endif()

add_executable(test_core_include test_core_include.cpp)
target_compile_features(test_core_include PRIVATE cxx_std_17)
target_compile_options(test_core_include PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_include_directories(test_core_include PRIVATE stubs ../src ${ESPRESSIO_SYSTEM_INCLUDE_DIR})
add_test(NAME CoreWithoutCommandOrSecurity COMMAND test_core_include)

if(NOT ESPRESSIO_COMMAND_INCLUDE_DIR)
    message(FATAL_ERROR "ESPRESSIO_COMMAND_INCLUDE_DIR is required for SocketCommand tests")
endif()

if(NOT ESPRESSIO_OBSERVABLE_INCLUDE_DIR)
    message(FATAL_ERROR "ESPRESSIO_OBSERVABLE_INCLUDE_DIR is required for observable Sockets tests")
endif()

add_executable(test_socket_command test_socket_command.cpp)
target_compile_features(test_socket_command PRIVATE cxx_std_17)
target_compile_options(test_socket_command PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_include_directories(test_socket_command PRIVATE ../src ${ESPRESSIO_SYSTEM_INCLUDE_DIR} ${ESPRESSIO_COMMAND_INCLUDE_DIR} ${ESPRESSIO_OBSERVABLE_INCLUDE_DIR})
add_test(NAME SocketCommand COMMAND test_socket_command)

if(NOT ESPRESSIO_SECURITY_INCLUDE_DIR)
    message(FATAL_ERROR "ESPRESSIO_SECURITY_INCLUDE_DIR is required for SocketSecurity tests")
endif()

add_executable(test_socket_security test_socket_security.cpp)
target_compile_features(test_socket_security PRIVATE cxx_std_17)
target_compile_options(test_socket_security PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_include_directories(test_socket_security PRIVATE ../src ${ESPRESSIO_SYSTEM_INCLUDE_DIR} ${ESPRESSIO_SECURITY_INCLUDE_DIR} ${ESPRESSIO_OBSERVABLE_INCLUDE_DIR})
add_test(NAME SocketSecurity COMMAND test_socket_security)

add_executable(test_socket_observable test_socket_observable.cpp)
target_compile_features(test_socket_observable PRIVATE cxx_std_17)
target_compile_options(test_socket_observable PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_include_directories(test_socket_observable PRIVATE ../src ${ESPRESSIO_SYSTEM_INCLUDE_DIR} ${ESPRESSIO_SECURITY_INCLUDE_DIR} ${ESPRESSIO_OBSERVABLE_INCLUDE_DIR})
add_test(NAME SocketObservable COMMAND test_socket_observable)

if(ESPRESSIO_TIMING_INCLUDE_DIR AND ESPRESSIO_UNITS_INCLUDE_DIR AND ESPRESSIO_OBSERVABLE_INCLUDE_DIR)
    add_executable(test_clock_sync_protocol test_clock_sync_protocol.cpp)
    target_compile_features(test_clock_sync_protocol PRIVATE cxx_std_17)
    target_compile_options(test_clock_sync_protocol PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_include_directories(test_clock_sync_protocol PRIVATE
        stubs
        ../src
        ../src/timing
        ${ESPRESSIO_SYSTEM_INCLUDE_DIR}
        ${ESPRESSIO_TIMING_INCLUDE_DIR}
        ${ESPRESSIO_UNITS_INCLUDE_DIR}
        ${ESPRESSIO_OBSERVABLE_INCLUDE_DIR}
    )
    add_test(NAME ClockSynchronizationProtocol COMMAND test_clock_sync_protocol)
endif()
