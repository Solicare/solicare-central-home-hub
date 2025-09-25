# 1. CMake 최소 버전 및 프로젝트 선언
cmake_minimum_required(VERSION 3.16)
project(solicare_central_home_hub VERSION 1.0.0 LANGUAGES CXX)

# 2. 빌드 타입 및 C++ 표준 설정
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 3. vcpkg 및 툴체인/트립렛 설정
if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if (DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
                CACHE STRING "vcpkg toolchain file")
        message(STATUS "Using vcpkg toolchain from VCPKG_ROOT: ${CMAKE_TOOLCHAIN_FILE}")
    endif ()
endif ()
if (NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "vcpkg triplet")
    message(STATUS "Using vcpkg triplet: ${VCPKG_TARGET_TRIPLET}")
endif ()

# 4. 컴파일러 정보 메시지
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

# 5. 라이브러리/패키지 찾기
find_package(OpenCV REQUIRED)
# OpenCV CUDA 지원 여부 확인
if (DEFINED OpenCV_WITH_CUDA)
    set(OPENCV_CUDA_STATUS ${OpenCV_WITH_CUDA})
elseif (DEFINED OpenCV_CUDA_VERSION)
    set(OPENCV_CUDA_STATUS "YES (version: ${OpenCV_CUDA_VERSION})")
else ()
    set(OPENCV_CUDA_STATUS "UNKNOWN (OpenCVConfig에서 CUDA 변수 미제공)")
endif ()

find_package(magic_enum CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(TBB REQUIRED)
find_package(fmt REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# 6. 소스/헤더 파일 수집
file(GLOB_RECURSE SRC_FILES "src/*.cpp")
file(GLOB_RECURSE HEADER_FILES "include/*.hpp")

# 7. 타겟 생성
add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEADER_FILES})

# 8. 타겟 include 디렉토리 설정
# (BOOST_ROOT 환경변수 있을 때만 추가)
target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        $<$<BOOL:${BOOST_ROOT}>:$ENV{BOOST_ROOT}>
)

# 9. 타겟 라이브러리 링크
# (필수 라이브러리)
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        ${OpenCV_LIBS}
        magic_enum::magic_enum
        nlohmann_json::nlohmann_json
        TBB::tbb
        fmt::fmt
        OpenSSL::SSL
        OpenSSL::Crypto
)

# 10. 플랫폼별 추가 라이브러리 (Windows)
if (WIN32)
    target_link_libraries(${PROJECT_NAME}
            PRIVATE
            ws2_32
            wsock32
            mswsock
    )
endif ()

# 11. 컴파일러별 옵션
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

# 12. 최종 메시지 출력
message(STATUS "=== Configuration Summary ===")
message(STATUS "Project: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "OpenCV Version: ${OpenCV_VERSION}")
message(STATUS "OpenCV CUDA Support: ${OPENCV_CUDA_STATUS}")
message(STATUS "=============================")
