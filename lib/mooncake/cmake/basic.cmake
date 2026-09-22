# Basic CMakeLists.txt

project(mooncake)

set(CMAKE_CXX_STANDARD 17)
cmake_policy(SET CMP0077 NEW)

# Src files
file(GLOB_RECURSE MOONCAKE_SRCS
    ${MOONCAKE_ROOT_DIR}/src/*.c
    ${MOONCAKE_ROOT_DIR}/src/*.cc
    ${MOONCAKE_ROOT_DIR}/src/*.cpp
)
# Include
set(MOONCAKE_INCS
    ${MOONCAKE_ROOT_DIR}/src/
)


add_library(${PROJECT_NAME} ${MOONCAKE_SRCS})
target_include_directories(${PROJECT_NAME} PUBLIC ${MOONCAKE_INCS})


option(MOONCAKE_BUILD_EXAMPLE "Build example" ON)


# Example
if(MOONCAKE_BUILD_EXAMPLE)
    add_subdirectory(./example/)
endif()



# CTest
enable_testing()

add_test(basic_demo example/basic_demo)
add_test(ability_manager_test example/ability_manager_test)
add_test(extension_test example/extension_test)
add_test(singleton_test example/singleton_test)
