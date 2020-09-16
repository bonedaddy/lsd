#####################
# START CMAKE SETUP #
#####################

# NOTE: you may want to customize the following to suit your environment
# it is intended for C and C++ projects using C17 and C++20

cmake_minimum_required(VERSION 3.0)
project(c-template LANGUAGES C CXX)
set(CMAKE_C_COMPILER /usr/bin/gcc)


# handle debug build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # -g indicates to include debug symbols
    list(APPEND flags -std=c17 -Wall -Wextra -Werror -pedantic -g)
    list(APPEND cxx-flags -std=c++2a -Wall -Wextra -Werror -pedantic -g)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Analyze")
    # override gcc compiler to v10 for static analysis
    set(CMAKE_C_COMPILER /usr/bin/gcc-10)
    list(APPEND flags -std=c17 -Wall -Wextra -Werror -pedantic -g -fanalyzer)
    list(APPEND cxx-flags -std=c++2a -Wall -Wextra -Werror -pedantic -g -fanalyzer)
    message(STATUS "enabling static analysis")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "CI")
    set(CMAKE_BUILD_TYPE "Release")
    list(APPEND flags -std=c17 -Wall -Wextra -Werror -pedantic -I/usr/local/lib)
    list(APPEND cxx-flags -std=c++2a -Wall -Wextra -Werror -pedantic -I/usr/local/lib)
    # override the previous C compiler choice
    set(CMAKE_C_COMPILER /usr/bin/gcc-9)
endif()


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
    list(APPEND flags -std=c17 -Wall -Wextra -Werror -pedantic)
    list(APPEND cxx-flags -std=c++2a -Wall -Wextra -Werror -pedantic)
endif()

include(cmake/ulog.cmake)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(lora 
    SHARED
        ./include/lora.h
        ./src/lora.c
)

add_executable(lora-cli ./main.c)
target_link_libraries(lora-cli lora wiringPi pthread)