cmake_minimum_required(VERSION 3.6)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/src/cmake)


if ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
    message( FATAL_ERROR "In-source builds not allowed. Please make a new directory and run CMake from there. You may need to remove CMakeCache.txt." )
endif()

set(PROJECT_NAME BLKCached)
project(${PROJECT_NAME})

find_package(TBB REQUIRED)
find_package(Boost COMPONENTS program_options filesystem REQUIRED)
find_package(pmemblk REQUIRED)


# TODO: uncomment if have unit tests
# enable_testing()

find_program( CLANG_TIDY NAMES clang-tidy)
# NOTE: you can add search paths for example `PATHS /usr/local/opt/llvm/bin/`

message("\n--------------------------------\n")
message("PROJECT NAME:\t\t${PROJECT_NAME}")
message("CMAKE_SYSTEM_NAME:\t${CMAKE_SYSTEM_NAME}")
message("CMAKE_C_COMPILER:\t${CMAKE_C_COMPILER}")
message("CMAKE_CXX_COMPILER:\t${CMAKE_CXX_COMPILER}")
message("CMAKE_GENERATOR:\t${CMAKE_GENERATOR}")
message("CMAKE_BUILD_TYPE:\t${CMAKE_BUILD_TYPE}")
message("CMAKE_BINARY_DIR:\t${CMAKE_BINARY_DIR}")
message("CMAKE_MODULE_PATH:\t${CMAKE_MODULE_PATH}")
message("CMAKE_PREFIX_PATH:\t${CMAKE_PREFIX_PATH}")
message("clang-tidy:\t\t${CLANG_TIDY}")
message("\n--------------------------------\n")



set(${PROJECT_NAME}_sources main.cpp)

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_sources})
target_include_directories(${PROJECT_NAME} PRIVATE ${TBB_INCLUDE_DIRS} PRIVATE ${Boost_INCLUDE_DIRS} PRIVATE ${libpmemblk_INCLUDES} )

# NOTE: this will slow down compilation, but you'll have static code analysis :)
if(CLANG_TIDY)
    set_property(
        TARGET ${PROJECT_NAME}
        PROPERTY CXX_CLANG_TIDY "${CLANG_TIDY}")
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(${PROJECT_NAME} ${TBB_LIBRARIES} ${Boost_LIBRARIES} ${libpmemblk_LIBRARIES} ${CMAKE_DL_LIBS} )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bds.conf ${CMAKE_CURRENT_BINARY_DIR}/bds.conf COPYONLY)
add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/src/bds.conf
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/bds.conf)