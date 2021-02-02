cmake_minimum_required( VERSION 3.12 )

project(mu_stdlib
	VERSION 1.7.8)

include(CMakeParseArguments)

option(MU_STDLIB_BUILD_TESTS "Build tests." OFF)

# ---- Add dependencies via CPM ----
# see https://github.com/TheLartians/CPM.cmake for more info

include(ExternalProject)

####

if (NOT DEFINED CPM_SCRIPTS)
	include(cmake/CPM.cmake)

	# One frustrating part of this project setup is the circular dependency between cpmpckg and this repo.
	# GIT_TAG will always lag cpmpckg @ HEAD when this project is updated there.
	CPMAddPackage(
		NAME cpmpckg
		GITHUB_REPOSITORY loopunit/cpmpckg
		GIT_TAG 9d5271a66365729b54588d166ec672d7db0a039c
		DOWNLOAD_ONLY true)

	include(${cpmpckg_SOURCE_DIR}/cmake/add_cpm_module.cmake)
else()
	include(${CPM_SCRIPTS}/CPM.cmake)
	include(${CPM_SCRIPTS}/add_cpm_module.cmake)
endif()

####

CPMAddPackage(
  NAME PackageProject.cmake
  GITHUB_REPOSITORY loopunit/PackageProject.cmake
  GIT_TAG e5ec20069766f4f078f9f01a86e250e20da0817c)

####

CPMAddBaseModule(boostorg_leaf)

set(mu_stdlib_SOURCE_ROOT ${CMAKE_CURRENT_LIST_DIR})

file(GLOB mu_stdlib_headers
	"${mu_stdlib_SOURCE_ROOT}/include/*.h")

file(GLOB mu_stdlib_sources
	"${mu_stdlib_SOURCE_ROOT}/src/*.cpp"
	"${mu_stdlib_SOURCE_ROOT}/src/*.h")

add_library(mu_stdlib STATIC ${mu_stdlib_sources} ${mu_stdlib_headers})

target_include_directories(mu_stdlib
	PUBLIC 
		$<BUILD_INTERFACE:${mu_stdlib_SOURCE_ROOT}/include>
		$<INSTALL_INTERFACE:include>)

target_link_libraries(mu_stdlib
	PUBLIC
		cpm_runtime::boostorg_leaf)

set_target_properties(mu_stdlib PROPERTIES CXX_STANDARD 17)

packageProject(
	NAME mu_stdlib
	VERSION ${PROJECT_VERSION}
	BINARY_DIR ${PROJECT_BINARY_DIR}
	INCLUDE_DIR ${mu_stdlib_SOURCE_ROOT}/include
	INCLUDE_DESTINATION include)

add_library(cpm_runtime::mu_stdlib ALIAS mu_stdlib)

if(MU_STDLIB_BUILD_TESTS)
	function(add_local_test)	
	    cmake_parse_arguments(
			arg
			""
			"TARGET_NAME"
		    "SOURCES"
	        ${ARGN}
		)
		add_executable(${arg_TARGET_NAME}
			${arg_SOURCES})

		set_target_properties(${arg_TARGET_NAME} PROPERTIES CXX_STANDARD 17)

		target_include_directories(${arg_TARGET_NAME}
			PRIVATE 
				${mu_stdlib_SOURCE_ROOT}/tests)

		target_link_libraries(${arg_TARGET_NAME} 
			PUBLIC
				cpm_runtime::mu_stdlib)
	endfunction()
	
	add_local_test(
		TARGET_NAME singleton_basic 
		SOURCES
			${CMAKE_CURRENT_LIST_DIR}/tests/singleton_basic.cpp)

	add_local_test(
		TARGET_NAME singleton_basic_dependencies 
		SOURCES
			${CMAKE_CURRENT_LIST_DIR}/tests/singleton_basic_dependencies.cpp)

	add_local_test(
		TARGET_NAME singleton_external
		SOURCES
			${CMAKE_CURRENT_LIST_DIR}/tests/singleton_external.cpp)

	add_local_test(
		TARGET_NAME singleton_external_dependencies
		SOURCES
			${CMAKE_CURRENT_LIST_DIR}/tests/singleton_external_dependencies.cpp)

	add_local_test(
		TARGET_NAME singleton_virtual
		SOURCES
			${CMAKE_CURRENT_LIST_DIR}/tests/singleton_virtual.cpp)

	add_local_test(
		TARGET_NAME singleton_virtual_dependencies
		SOURCES
			${CMAKE_CURRENT_LIST_DIR}/tests/singleton_virtual_dependencies.cpp)

	add_local_test(
		TARGET_NAME hello
		SOURCES
			${CMAKE_CURRENT_LIST_DIR}/tests/hello.cpp)
endif()
