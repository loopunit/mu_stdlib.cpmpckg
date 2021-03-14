cmake_minimum_required( VERSION 3.12 )

project(mu_stdlib
	VERSION 1.7.8)

include(CMakeParseArguments)

option(MU_STDLIB_BUILD_TESTS "Build tests." OFF)

# ---- Add dependencies via CPM ----
# see https://github.com/TheLartians/CPM.cmake for more info

include(ExternalProject)

####

if (NOT DEFINED cpmpckg_SOURCE_DIR)
	include(cmake/CPM.cmake)

	# One frustrating part of this project setup is the circular dependency between cpmpckg and this repo.
	# GIT_TAG will always lag cpmpckg @ HEAD when this project is updated there.
	CPMAddPackage(
		NAME cpmpckg
		GITHUB_REPOSITORY loopunit/cpmpckg
		GIT_TAG 72ee31a763dca7ba16557e481da81d0644658caf
		DOWNLOAD_ONLY true)

	include(${cpmpckg_SOURCE_DIR}/cmake/add_cpm_module.cmake)
else()
	set(CPM_SCRIPTS ${cpmpckg_SOURCE_DIR}/cmake)
	include(${cpmpckg_SOURCE_DIR}/cmake/CPM.cmake)
	include(${cpmpckg_SOURCE_DIR}/cmake/add_cpm_module.cmake)
endif()

####

CPMAddPackage(
  NAME PackageProject.cmake
  GITHUB_REPOSITORY loopunit/PackageProject.cmake
  GIT_TAG e5ec20069766f4f078f9f01a86e250e20da0817c)

####

CPMAddBaseModule(boostorg_leaf)

CPMAddPackage(
	NAME scope_guard
	GITHUB_REPOSITORY ricab/scope_guard
	GIT_TAG 760de0a9ea0fec980a5244604ffa5b1acff2db7a
	DOWNLOAD_ONLY true)

if( NOT ${scope_guard_ADDED} )
	message( FATAL_ERROR "Could not find scope_guard" )
endif()

set(mu_stdlib_SOURCE_ROOT ${CMAKE_CURRENT_LIST_DIR})

file(GLOB mu_stdlib_headers
	"${mu_stdlib_SOURCE_ROOT}/include/*.h")

file(GLOB mu_stdlib_sources
	"${mu_stdlib_SOURCE_ROOT}/src/*.cpp"
	"${mu_stdlib_SOURCE_ROOT}/src/*.h")

add_library(mu_stdlib STATIC ${mu_stdlib_sources} ${mu_stdlib_headers})

target_include_directories(mu_stdlib
	PUBLIC 
		$<BUILD_INTERFACE:${scope_guard_SOURCE_DIR}>
		$<BUILD_INTERFACE:${mu_stdlib_SOURCE_ROOT}/include>
		$<INSTALL_INTERFACE:include>)

target_link_libraries(mu_stdlib
	PUBLIC
		cpm_install::boostorg_leaf)

set_target_properties(mu_stdlib PROPERTIES CXX_STANDARD 17)

packageProject(
	NAME mu_stdlib
	VERSION ${PROJECT_VERSION}
	BINARY_DIR ${PROJECT_BINARY_DIR}
	INCLUDE_DIR ${mu_stdlib_SOURCE_ROOT}/include
	INCLUDE_DESTINATION include)

install(
	FILES ${scope_guard_SOURCE_DIR}/scope_guard.hpp
	DESTINATION ${mu_stdlib_SOURCE_ROOT}/include)

add_library(cpm_install::mu_stdlib ALIAS mu_stdlib)

if (CPM_BUILD_TEST)
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
				cpm_install::mu_stdlib)
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
