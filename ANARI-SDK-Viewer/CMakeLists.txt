CMAKE_MINIMUM_REQUIRED(VERSION 3.21)

SET (example_dir ${CMAKE_CURRENT_LIST_DIR})
GET_FILENAME_COMPONENT(example_target "${example_dir}" NAME)

IF (TARGET ${example_target})
    MESSAGE(WARNING "Example ${example_target} already exists, skipping BUILD_EXAMPLE step for this target")
    RETURN()
ENDIF()

MESSAGE(STATUS "Building example ${example_target} in ${example_dir}")
FILE(GLOB_RECURSE example_src_files
        # header files
        "${example_dir}/src/**.h"
        "${example_dir}/src/**.hh"
        "${example_dir}/src/**.hpp"
        "${example_dir}/src/**.hxx"
        "${example_dir}/src/**.ixx" # Cpp20
        # source files
        "${example_dir}/src/**.c"
        "${example_dir}/src/**.cc"
        "${example_dir}/src/**.cpp"
        "${example_dir}/src/**.cxx"
        "${example_dir}/src/**.cppm" # Cpp20
    )

IF (WIN32)
    ADD_EXECUTABLE(${example_target} WIN32 ${example_src_files})
ELSE ()
    ADD_EXECUTABLE(${example_target} ${example_src_files})
ENDIF ()

SET_TARGET_PROPERTIES(${example_target}
    PROPERTIES
        SOURCE_DIR ${example_dir}
    )

TARGET_INCLUDE_DIRECTORIES(${example_target} 
    PRIVATE
        ${example_dir}/
        ${example_dir}/src/
    PUBLIC
        ${GENERATED_SOURCE_FILES_DIR}
    )

TARGET_COMPILE_OPTIONS(${example_target}
    PRIVATE
#        $<$<CXX_COMPILER_ID:MSVC>:/WX /W4>
#        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wconversion -Werror>
    )
TARGET_COMPILE_DEFINITIONS(${example_target}
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
    )

# from ~vendor dir
TARGET_INCLUDE_DIRECTORIES(${example_target} PRIVATE
        ${CMAKE_SOURCE_DIR}/~vendor/include
    )

# VcPkg packages
IF (NOT TARGET fmt::fmt)
    FIND_PACKAGE(fmt CONFIG REQUIRED)
ENDIF ()
IF (NOT TARGET glfw)
    FIND_PACKAGE(glfw3 CONFIG REQUIRED)
ENDIF ()
IF (NOT TARGET glad::glad)
    FIND_PACKAGE(glad CONFIG REQUIRED)
ENDIF ()
IF (NOT TARGET imgui::imgui)
    FIND_PACKAGE(imgui REQUIRED)
ENDIF ()
IF (NOT TARGET OpenGL::GL)
    set(OpenGL_GL_PREFERENCE "LEGACY")
    find_dependency(OpenGL 4 REQUIRED)
ENDIF ()
IF (NOT TARGET nfd::nfd)
    FIND_PACKAGE(nfd REQUIRED)
ENDIF ()
IF (NOT DEFINED Stb_INCLUDE_DIR OR NOT EXISTS ${Stb_INCLUDE_DIR})
    FIND_PACKAGE(Stb REQUIRED)
ENDIF ()
IF (NOT TARGET unofficial::tinyexr::tinyexr)
    FIND_PACKAGE(tinyexr CONFIG REQUIRED)
ENDIF ()
IF (NOT TARGET anari::anari)
    FIND_PACKAGE(anari REQUIRED)
ENDIF ()
TARGET_LINK_LIBRARIES(${example_target}
    PRIVATE
        fmt::fmt
        glfw
        glad::glad
        imgui::imgui OpenGL::GL
        nfd::nfd
        unofficial::tinyexr::tinyexr
        anari::anari anari::anari_test_scenes
    )

# tests target
ADD_TESTS_FOR_TARGET(${example_target}
    INCLUDE_DIRECTORIES
    COMPILE_DEFINITIONS
    COMPILE_OPTIONS
    LINK_LIBRARIES
    LINK_DIRECTORIES)

# copy if different all resources in ./resources folder
FILE(GLOB_RECURSE example_resource_files
        "${example_dir}/resources/**"
        "${CMAKE_SOURCE_DIR}/~vendor/resources/**"
    )
FOREACH (resource_file ${example_resource_files})
    ADD_CUSTOM_COMMAND(
        TARGET ${example_target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${resource_file}
            $<TARGET_FILE_DIR:${example_target}>
        COMMENT "Copying resource file ${resource_file}"
    )
ENDFOREACH()

# Install the example to the install directory
INSTALL_TARGET_AND_ITS_DEPENDENCIES(${example_target} "./bin")

