# copy property from target to target
# params:
#   target_from: the target to copy from
#   target_to: the target to copy to
#   property_name: the property to copy
FUNCTION(COPY_PROPERTY_FROM_TARGET_TO_TARGET target_from target_to property_name)
    IF (NOT target_from)
		MESSAGE(FATAL_ERROR "COPY_PROPERTY_FROM_TARGET_TO_TARGET must be called with a target_from argument")
    ELSEIF (NOT target_to)
		MESSAGE(FATAL_ERROR "COPY_PROPERTY_FROM_TARGET_TO_TARGET must be called with a target_to argument")
    ELSEIF (NOT property_name)
        MESSAGE(FATAL_ERROR "COPY_PROPERTY_FROM_TARGET_TO_TARGET must be called with a property_name argument")
    ENDIF ()

    GET_PROPERTY(the_property TARGET ${target_from} PROPERTY ${property_name})
    IF (NOT the_property STREQUAL "the_property-NOTFOUND")
        SET_PROPERTY(TARGET ${target_to} PROPERTY ${property_name} ${the_property})
    ENDIF ()
ENDFUNCTION()

# add test target for target from source-files
# params:
#   target: the target to build tests for
#   tests_target: the name of the test target
#   tests_src_files: array, the source-files of the test target
#   ...: the properties to copy from target to tests_target
FUNCTION(ADD_TESTS_TARGET_FOR_TARGET_FROM_SRC_FILES target tests_target tests_src_files)
    IF (NOT target)
        MESSAGE(FATAL_ERROR "ADD_TESTS_TARGET_FOR_TARGET_FROM_SRC_FILES must be called with a target argument")
    ELSEIF (NOT tests_target)
        MESSAGE(FATAL_ERROR "ADD_TESTS_TARGET_FOR_TARGET_FROM_SRC_FILES must be called with a tests_target argument")
    ELSEIF (NOT tests_src_files)
        MESSAGE(FATAL_ERROR "ADD_TESTS_TARGET_FOR_TARGET_FROM_SRC_FILES must be called with a tests_src_files argument")
    ENDIF ()

    MESSAGE(STATUS "Building test target for ${target}")

    # add test target
    ADD_EXECUTABLE(${tests_target} ${tests_src_files})
    IF (NOT TARGET Boost::unit_test_framework)
        FIND_PACKAGE(Boost REQUIRED COMPONENTS unit_test_framework)
    ENDIF ()

    TARGET_LINK_LIBRARIES(${tests_target} PRIVATE Boost::unit_test_framework)

    # copy properties from target to tests_target
    FOREACH (property_name IN LISTS ARGN)
        COPY_PROPERTY_FROM_TARGET_TO_TARGET(${target} ${tests_target} ${property_name})
    ENDFOREACH ()

    GET_TARGET_PROPERTY(target_type ${target} TYPE)
    IF (target_type MATCHES "_+LIBRARY")
		TARGET_LINK_LIBRARIES(${tests_target} PRIVATE ${target})
    ENDIF()

    ADD_TEST(NAME ${tests_target} COMMAND ${tests_target})
ENDFUNCTION()

# add test target for target
# params:
#   target: the target to build tests for
#   tests_target: the name of the test target
#   tests_src_dir: the source-dir of the target
#   ...: the properties to copy from target to tests_target
FUNCTION(ADD_TESTS_TARGET_FOR_TARGET_FROM_DIR target tests_target tests_src_dir)
    IF (NOT target)
        MESSAGE(FATAL_ERROR "ADD_TESTS_TARGET_FOR_TARGET_FROM_DIR must be called with a target argument")
    ELSEIF (NOT tests_target)
        MESSAGE(FATAL_ERROR "ADD_TESTS_TARGET_FOR_TARGET_FROM_DIR must be called with a tests_target argument")
    ELSEIF (NOT tests_src_dir)
        MESSAGE(FATAL_ERROR "ADD_TESTS_TARGET_FOR_TARGET_FROM_DIR must be called with a tests_src_dir argument")
    ENDIF ()
    
    # get the source-dir of the target
    IF (tests_src_dir STREQUAL "")
        MESSAGE(FATAL_ERROR "tests_src_dir must be set")
    ENDIF ()

    MESSAGE(STATUS "Building test target for ${target}")

    # collect <source-dir>/tests/**/*.cpp files
    FILE(GLOB_RECURSE tests_src_files
            "${tests_src_dir}/**.c"
            "${tests_src_dir}/**.cc"
            "${tests_src_dir}/**.cpp"
            "${tests_src_dir}/**.cxx"
            "${tests_src_dir}/**.cppm"
            )

    ADD_TESTS_TARGET_FOR_TARGET_FROM_SRC_FILES(${target} ${tests_target} ${tests_src_files} ${ARGN})
ENDFUNCTION()

# add test target for target from "./tests" dir
# params:
#   target: the target to build tests for
#   tests_target: the name of the test target
#   ...: the properties to copy from target to tests_target
FUNCTION(ADD_TESTS_TARGET_FOR_TARGET target tests_target)
    IF (NOT target)
		MESSAGE(FATAL_ERROR "ADD_TESTS_TARGET_FOR_TARGET must be called with a target argument")
    ELSEIF (NOT tests_target)
        MESSAGE(FATAL_ERROR "ADD_TESTS_TARGET_FOR_TARGET must be called with a tests_target argument")
    ENDIF ()

    GET_TARGET_PROPERTY(target_src_dir ${target} SOURCE_DIR)
    IF (target_src_dir STREQUAL "target_src_dir-NOTFOUND")
        MESSAGE(WARNING "Target ${target} does not have SOURCE_DIR property set, will not build tests for it")
        RETURN()
    ENDIF ()

    # collect <source-dir>/tests/**/*.cpp files
    FILE(GLOB_RECURSE tests_src_files
            "${target_src_dir}/tests/**.c"
            "${target_src_dir}/tests/**.cc"
            "${target_src_dir}/tests/**.cpp"
            "${target_src_dir}/tests/**.cxx"
            "${target_src_dir}/tests/**.cppm"
            "${target_src_dir}/unit_tests.c"
            "${target_src_dir}/unit_tests.cc"
            "${target_src_dir}/unit_tests.cpp"
            "${target_src_dir}/unit_tests.cxx"
            "${target_src_dir}/unit_tests.cppm"
        )
    ADD_TESTS_TARGET_FOR_TARGET_FROM_SRC_FILES(${target} ${tests_target} ${tests_src_files} ${ARGN})
ENDFUNCTION()

# add tests for target from "./tests" dir
# params:
#   target: the target to build tests for
#   ...: the properties to copy from target to tests_target
FUNCTION(ADD_TESTS_FOR_TARGET target)
    IF (NOT target)
        MESSAGE(FATAL_ERROR "ADD_TESTS_FOR_TARGET must be called with a target argument")
    ENDIF ()

    GET_TARGET_PROPERTY(target_name ${target} NAME)
	ADD_TESTS_TARGET_FOR_TARGET(${target} ${target_name}_tests ${ARGN})
ENDFUNCTION()

# add tests for target from dir
# params:
#   target: the target to build tests for
#   tests_src_dir: the source-dir of the target
#   ...: the properties to copy from target to tests_target
FUNCTION(ADD_TESTS_FOR_TARGET_FROM_DIR target tests_src_dir)
    IF (NOT target)
        MESSAGE(FATAL_ERROR "ADD_TESTS_FOR_TARGET_FROM_DIR must be called with a target argument")
    ELSEIF (NOT tests_src_dir)
        MESSAGE(FATAL_ERROR "ADD_TESTS_FOR_TARGET_FROM_DIR must be called with a tests_src_dir argument")
    ENDIF ()

    GET_TARGET_PROPERTY(target_name ${target} NAME)
    ADD_TESTS_TARGET_FOR_TARGET_FROM_DIR(${target} ${target_name}_tests ${tests_src_dir} ${ARGN})
ENDFUNCTION()

# find vcpkg toolchain file
MACRO(FIND_VCPKG)
    # START: FIND VCPKG TOOLCHAIN FILE
    SET(NO_VCPKG_TOOLCHAIN_FILE TRUE)
    FOREACH (THE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE} ${CMAKE_TOOLCHAIN_FILE_OVERRIDE})
        IF (${THE_TOOLCHAIN_FILE} MATCHES "vcpkg.cmake" AND EXISTS ${THE_TOOLCHAIN_FILE})
            SET(NO_VCPKG_TOOLCHAIN_FILE FALSE)
        ENDIF ()
        IF (NOT NO_VCPKG_TOOLCHAIN_FILE)
            break () # there is a vcpkg toolchain file
        ENDIF ()
    ENDFOREACH ()
    IF (${NO_VCPKG_TOOLCHAIN_FILE})
        IF (DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
            LIST(APPEND CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        ELSEIF (DEFINED ENV{VCPKG_DIR} AND EXISTS "$ENV{VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake")
            LIST(APPEND CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake")
        ELSE ()
            MESSAGE(FATAL_ERROR "No VcPkg toolchain file found")
        ENDIF ()
    ENDIF ()
    ## END: FIND VCPKG TOOLCHAIN FILE
    
    # Read the vcpkg.json file
    FILE(READ "${CMAKE_SOURCE_DIR}/vcpkg.json" VCPKG_MANIFEST_RAW)
    # Parse "version-string" from JSON content
    STRING(JSON VCPKG_MANIFEST_VERSION_STRING GET ${VCPKG_MANIFEST_RAW} "version-string")
ENDMACRO()

# install target and dependencies
# params:
#   target: the target to install
#   relative_install_dir (optional): the relative sub directory to install to
FUNCTION(INSTALL_TARGET_AND_ITS_DEPENDENCIES target relative_install_dir)
    # Install the target
    IF (NOT relative_install_dir)
        SET(relative_install_dir "./")
    ENDIF ()

    # Check if the target is an executable
    GET_TARGET_PROPERTY(target_type ${target} TYPE)
    IF (target_type STREQUAL "EXECUTABLE")
        INSTALL(TARGETS ${target} RUNTIME DESTINATION ${relative_install_dir})

        # Install the dependencies to the install directory
        GET_PROPERTY(this_target_linked_libs TARGET ${target} PROPERTY LINK_LIBRARIES)
        FOREACH (lib IN LISTS this_target_linked_libs)
            GET_TARGET_PROPERTY(lib_type ${lib} TYPE)
            IF (lib_type STREQUAL "INTERFACE_LIBRARY" OR lib_type STREQUAL "STATIC_LIBRARY")
                CONTINUE()
            ENDIF ()

            SET(lib_location)
            IF (NOT lib_location)
                STRING(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER)
                GET_TARGET_PROPERTY(lib_location ${lib} LOCATION_${CMAKE_BUILD_TYPE_UPPER})
            ENDIF ()
            IF (NOT lib_location)
				GET_TARGET_PROPERTY(lib_location ${lib} LOCATION)
            ENDIF ()

            IF (lib_location)
                INSTALL(FILES ${lib_location} DESTINATION ${relative_install_dir} CONFIGURATIONS ${CMAKE_BUILD_TYPE})
            ENDIF ()
        ENDFOREACH ()
    ELSE ()
        MESSAGE (FATAL_ERROR "Target ${target} is not an executable")
        # TODO: support other target types

    ENDIF ()
ENDFUNCTION()

# create packages for distribution
MACRO(CREATE_PACKAGES_VIA_CPACK)
    INCLUDE(InstallRequiredSystemLibraries)
	
    IF (NOT CPACK_PACKAGE_DIRECTORY)
        SET(CPACK_PACKAGE_DIRECTORY "${CMAKE_SOURCE_DIR}/!installers")
    ENDIF ()
    
    IF (NOT CPACK_PACKAGE_VENDOR)
		SET(CPACK_PACKAGE_VENDOR "Unknown Vendor")
    ENDIF ()

    IF (NOT CPACK_PACKAGE_CONTACT)
        SET(CPACK_PACKAGE_CONTACT "${CPACK_PACKAGE_VENDOR} <no email provided>")
    ENDIF ()

    IF (NOT CPACK_PACKAGE_DESCRIPTION_SUMMARY)
        SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The ${PROJECT_NAME} project, contact: ${CPACK_PACKAGE_CONTACT} for any issues.")
    ENDIF ()

	IF (NOT CPACK_PACKAGE_HOMEPAGE_URL)
        SET(CPACK_PACKAGE_HOMEPAGE_URL  "www.example.com")
    ENDIF ()

	SET(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
    SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
	SET(CPACK_RESOURCE_FILE_README  "${CMAKE_SOURCE_DIR}/README.md")
	SET(CPACK_RESOURCE_FILE_WELCOME "${CMAKE_SOURCE_DIR}/README.md")
    SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
    SET(CPACK_PACKAGE_VERSION_MAJOR "${${PROJECT_NAME}_VERSION_MAJOR}")
    SET(CPACK_PACKAGE_VERSION_MINOR "${${PROJECT_NAME}_VERSION_MINOR}")
    SET(CPACK_PACKAGE_VERSION_PATCH "${${PROJECT_NAME}_VERSION_PATCH}")
    SET(CPACK_PACKAGE_VERSION       "${${PROJECT_NAME}_VERSION}")

    SET(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
	SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

    # For creating installers for Platforms
    IF (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        SET(CPACK_GENERATOR "ZIP")
    
        FIND_PACKAGE(WIX)
        FIND_PACKAGE(NSIS)
        IF (WIX_FOUND)
            list(APPEND CPACK_GENERATOR "WIX")
        ENDIF(WIX_FOUND)
        IF (NSIS_FOUND) 
            list(APPEND CPACK_GENERATOR "NSIS")
        ENDIF(NSIS_FOUND)
    ELSEIF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        SET(CPACK_GENERATOR "TGZ")

        FIND_PROGRAM(DPKG_PROGRAM dpkg DOC "dpkg program of Debian-based systems")
        IF(DPKG_PROGRAM)
            SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
            SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
            EXECUTE_PROCESS(COMMAND ${DPKG_PROGRAM} --print-architecture
				    OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
				    OUTPUT_STRIP_TRAILING_WHITESPACE)
            SET(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}_${${PROJECT_NAME}_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
            LIST(APPEND CPACK_GENERATOR "DEB")
        ENDIF(DPKG_PROGRAM)
    ELSE()
        MESSAGE(FATAL_ERROR "Unsupported platform")
    ENDIF ()

    MESSAGE("CPACK_GENERATORS: ${CPACK_GENERATOR}")
    INCLUDE(CPack)
ENDMACRO()