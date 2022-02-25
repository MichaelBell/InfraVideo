# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_COPY_TO_RAM 1)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/pi/pico/pico-sdk")

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)

project(InfraVideo C CXX)

# Initialise the Pico SDK
pico_sdk_init()

add_subdirectory(mlx90640)
add_subdirectory(PicoDVI/software/libdvi)
include_directories(PicoDVI/software/include)

# Add executable. Default name is the project name, version 0.1

add_executable(InfraVideo InfraVideo.cpp display.c )

target_compile_definitions(InfraVideo PRIVATE
  DVI_DEFAULT_SERIAL_CONFIG=pico_sock_cfg
  PICO_CORE1_STACK_SIZE=0x200
  )

pico_set_program_name(InfraVideo "InfraVideo")
pico_set_program_version(InfraVideo "0.1")

pico_enable_stdio_uart(InfraVideo 1)
pico_enable_stdio_usb(InfraVideo 0)

# Add the standard library to the build
target_link_libraries(InfraVideo pico_stdlib pico_multicore)

# Add any user requested libraries
target_link_libraries(InfraVideo
        hardware_i2c
        mlx90640
        libdvi
        )

pico_add_extra_outputs(InfraVideo)

