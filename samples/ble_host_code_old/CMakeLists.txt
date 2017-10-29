cmake_minimum_required (VERSION 2.6)
project (ble_host_app)

# Make all libraries be included into a static executable
SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
SET(BUILD_SHARED_LIBRARIES OFF)
SET(CMAKE_EXE_LINKER_FLAGS "-static")
SET(CMAKE_C_FLAGS "-std=gnu11")

#Added version number here
set (ble_host_app_VERSION_MAJOR 1)
set (ble_host_app_VERSION_MINOR 0)



# configure a header file to pass some of the CMake settings
# to the source code
#configure_file (
#  "${PROJECT_SOURCE_DIR}/ble_host_appConfig.h.in"
#  "${PROJECT_BINARY_DIR}/ble_host_appConfig.h"
#  )


LINK_DIRECTORIES("/usr/lib/x86_64-linux-gnu/")

INCLUDE_DIRECTORIES("/usr/include/openssl/")


#Following line is for specifying the compilation flags
#SET(GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")

#Following line is for specifying the linker flags
#SET(GCC_COVERAGE_LINK_FLAGS    "-L/usr/lib -lssl -lcrypto -lpthread")


#add_subdirectory(additionlib)
#add_subdirectory(subtractionlib)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")

#List of binaries as output of the build process
add_executable(ble_host_app.out main.c cmd_def.c stubs.c encrypt.c uart.c update_thread.c profile.c)

target_link_libraries(ble_host_app.out ssl crypto pthread)

# add the install targets
install (TARGETS ble_host_app.out DESTINATION "${PROJECT_SOURCE_DIR}/bin")
install (FILES "${PROJECT_BINARY_DIR}/ble_host_appConfig.h"        
         DESTINATION include)
         
         
include(CTest)

# does the application run
add_test (mainTests main)
