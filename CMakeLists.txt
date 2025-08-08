if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if (DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
                CACHE STRING "vcpkg toolchain file")
        message(STATUS "Using vcpkg toolchain from VCPKG_ROOT: ${CMAKE_TOOLCHAIN_FILE}")
    endif ()
endif ()

# Set vcpkg target triplet for MinGW
if (NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "vcpkg triplet")
    message(STATUS "Using vcpkg triplet: ${VCPKG_TARGET_TRIPLET}")
endif ()

cmake_minimum_required(VERSION 3.16)  # Updated to a more recent version
project(solicare_central_hub_launcher VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # Disable compiler-specific extensions

# Build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Find dependencies
find_package(OpenCV REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(TBB REQUIRED)

# Compiler identification
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

# Source files - automatically find all .cc files in src/
file(GLOB_RECURSE SRC_FILES "src/*.cc")
# Header files - automatically find all .h files in include/
file(GLOB_RECURSE HEADER_FILES "include/*.h")

# Define sources
set(SOURCES
        launcher.cc
        ${SRC_FILES}
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADER_FILES})

# Target-specific include directories (modern CMake approach)
target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        $<$<BOOL:${BOOST_ROOT}>:$ENV{BOOST_ROOT}>
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        ${OpenCV_LIBS}
        magic_enum::magic_enum
        TBB::tbb
)

# Windows-specific libraries for Boost.Asio
if (WIN32)
    target_link_libraries(${PROJECT_NAME}
            PRIVATE
            ws2_32
            wsock32
            mswsock
    )
endif ()

# Compiler-specific optimizations
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
            $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:/Od /Zi /W4>
            $<$<CONFIG:Release>:/O2 /DNDEBUG>
    )
endif ()

# Print configuration summary
message(STATUS "=== Configuration Summary ===")
message(STATUS "Project: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "OpenCV Version: ${OpenCV_VERSION}")
message(STATUS "=============================")
