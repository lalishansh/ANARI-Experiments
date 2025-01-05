CMAKE_MINIMUM_REQUIRED(VERSION 3.21)

SET (library_dir ${CMAKE_CURRENT_LIST_DIR})
GET_FILENAME_COMPONENT(library_target "${library_dir}" NAME)

IF (TARGET ${library_target})
    MESSAGE(WARNING "Example ${library_target} already exists, skipping BUILD_EXAMPLE step for this target")
    RETURN()
ENDIF()

MESSAGE(STATUS "Building example ${library_target} in ${library_dir}")
FILE(GLOB_RECURSE library_src_files
        # header files
        "${library_dir}/src/**.h"
        "${library_dir}/src/**.hh"
        "${library_dir}/src/**.hpp"
        "${library_dir}/src/**.hxx"
        "${library_dir}/src/**.ixx" # Cpp20
        # source files
        "${library_dir}/src/**.c"
        "${library_dir}/src/**.cc"
        "${library_dir}/src/**.cpp"
        "${library_dir}/src/**.cxx"
        "${library_dir}/src/**.cppm" # Cpp20
)

# public headers
SET(library_public_header_dir ${library_dir}/include)
FILE(GLOB_RECURSE library_public_header_files
        "${library_public_header_dir}/**.h"
        "${library_public_header_dir}/**.hh"
        "${library_public_header_dir}/**.hpp"
        "${library_public_header_dir}/**.hxx"
        "${library_public_header_dir}/**.ixx" # Cpp20
)


ADD_LIBRARY(${library_target} SHARED
        ${library_public_header_files}
        ${library_src_files})

SET_TARGET_PROPERTIES(${library_target}
        PROPERTIES
        SOURCE_DIR ${library_dir}
)

TARGET_INCLUDE_DIRECTORIES(${library_target}
        PRIVATE
        ${library_dir}/
        ${library_dir}/src/
        PUBLIC
        ${library_public_header_dir}
        ${GENERATED_SOURCE_FILES_DIR}
)

TARGET_COMPILE_OPTIONS(${library_target}
        PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/WX /W4 /wd4201> # disable warning C4201: nonstandard extension used: nameless struct/union
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wconversion -Wno-gnu-anonymous-struct>
)
TARGET_COMPILE_DEFINITIONS(${library_target}
        PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)

# from ~vendor dir
TARGET_INCLUDE_DIRECTORIES(${library_target} PRIVATE
        ${CMAKE_SOURCE_DIR}/~vendor/include
)

IF (ANDROID)
    TARGET_COMPILE_DEFINITIONS(${library_target} PRIVATE PLATFORM__ANDROID)
ELSEIF (WIN32)
    TARGET_COMPILE_DEFINITIONS(${library_target} PRIVATE PLATFORM__WINDOWS)
ELSEIF (APPLE)
    TARGET_COMPILE_DEFINITIONS(${library_target} PRIVATE PLATFORM__MACOS)
ELSEIF (UNIX)
    TARGET_COMPILE_DEFINITIONS(${library_target} PRIVATE PLATFORM__LINUX)
ENDIF ()


# VcPkg packages
IF (NOT TARGET fmt::fmt)
    FIND_PACKAGE(fmt CONFIG REQUIRED)
ENDIF ()
TARGET_LINK_LIBRARIES(${library_target}
        PRIVATE
        fmt::fmt
)

# tests target
SET(tests_target "${library_target}-tests")
ADD_TESTS_TARGET_FOR_TARGET(${library_target} ${tests_target}
        INCLUDE_DIRECTORIES
        COMPILE_DEFINITIONS
        COMPILE_OPTIONS
        LINK_LIBRARIES
        LINK_DIRECTORIES
)

# VcPkg packages
IF (NOT TARGET glfw)
    FIND_PACKAGE(glfw3 CONFIG REQUIRED)
ENDIF ()
IF (NOT TARGET Vulkan::Vulkan)
    FIND_PACKAGE(Vulkan REQUIRED)
ENDIF ()
TARGET_LINK_LIBRARIES(${tests_target}
    PRIVATE
        glfw Vulkan::Vulkan
)

# TODO: add install target for library and headers
INSTALL_LIBRARY_AND_ITS_DEPENDENCIES(${library_target} "./bin" "./lib" "./include")
