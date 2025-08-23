cmake_minimum_required(VERSION 3.10)
project(iCamera VERSION 1.0.0 LANGUAGES CXX C)

# Build option: native or cross
option(CROSS_COMPILE "Enable cross-compilation for ARM" ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wno-psabi)
# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Cross-compilation setup
if(CROSS_COMPILE)
    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_PROCESSOR arm)
    
    # Toolchain prefix
    set(TOOLCHAIN_PREFIX "arm-rockchip830-linux-uclibcgnueabihf")
    
    # Sysroot path
    set(CMAKE_SYSROOT "/home/quangtv98/workspace/camera/rockchip/sdk/luckfox-pico/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot")
    
    # Compilers
    set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
    set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
    
    # Find programs in the build host directories
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    
    # Search for libraries and headers in the target directories
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    
    # Set pkg-config to use cross-compilation
    set(PKG_CONFIG_EXECUTABLE ${TOOLCHAIN_PREFIX}-pkg-config)
    
    # Additional flags for uClibc
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv7-a -mfpu=neon -mfloat-abi=hard")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv7-a -mfpu=neon -mfloat-abi=hard")
    
    # Disable some features not available in uClibc
    add_definitions(-D_GNU_SOURCE)
    add_definitions(-D__USE_GNU)
    
    message(STATUS "Cross-compilation enabled for ARM")
else()
    message(STATUS "Native build enabled")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fPIC")
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)
# Debug flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -DDEBUG")

# Release flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/paho
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/source/core
    ${CMAKE_SOURCE_DIR}/source/app
    ${CMAKE_SOURCE_DIR}/source/common
    ${CMAKE_SOURCE_DIR}/source/utils
    ${CMAKE_SOURCE_DIR}/source/module/network
    ${CMAKE_SOURCE_DIR}/source/module/network/mqtt
    ${CMAKE_SOURCE_DIR}/source/module/audio
    ${CMAKE_SOURCE_DIR}/source/module/video
    ${CMAKE_SOURCE_DIR}/source/module/stream/rtsp
    ${CMAKE_SOURCE_DIR}/source/module/stream/rtmp
    ${CMAKE_SOURCE_DIR}/source/module/console
    ~/workspace/camera/rockchip/sdk/luckfox-pico/media/out/include
)

# Find required packages
find_package(Threads REQUIRED)

# Set library directories
link_directories(
    ${CMAKE_SOURCE_DIR}/lib
    ~/workspace/camera/rockchip/sdk/luckfox-pico/sysdrv/source/buildroot/buildroot-2023.02.6/output/target/usr/lib
    ~/workspace/camera/rockchip/sdk/luckfox-pico/media/out/lib
)

# Source files
file(GLOB_RECURSE SOURCES 
    "source/*.cpp"
    "source/*.c"
)

# Header files
file(GLOB_RECURSE HEADERS 
    "source/*.hpp"
    "source/*.h"
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ssl
    crypto
    paho-mqtt3cs
    z
    v4l2
    v4lconvert 
    gnutls
    nettle
    hogweed   
    gmp  
    unistring
    tasn1 
    drm
    iconv
    intl
    avformat
    avfilter
    avcodec
    avutil
    avdevice
    swresample
    rockit
    rockchip_mpp
    rga
    ${CMAKE_THREAD_LIBS_INIT}
)

# Set compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Install rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install headers
install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Install libraries
install(DIRECTORY lib/
    DESTINATION lib
    FILES_MATCHING PATTERN "*.so*" PATTERN "*.a"
)

# Print configuration info
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Cross-compilation: ${CROSS_COMPILE}")
message(STATUS "Source files:")
foreach(file ${SOURCES})
    message(STATUS "  ${file}")
endforeach()

message(STATUS "Include directories: ${CMAKE_SOURCE_DIR}/include")
message(STATUS "Library directories: ${CMAKE_SOURCE_DIR}/lib")
