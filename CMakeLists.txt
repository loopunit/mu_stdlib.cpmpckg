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
		GIT_TAG d92021769936a770f8eccc41b19e24f0bf606ff6
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

####

CPMAddBaseModule(cwalk)

####

CPMAddBaseModule(libuv)

####

CPMAddBaseModule(taskflow)

####

CPMAddPackage(
	NAME gsl_lite
	GITHUB_REPOSITORY gsl-lite/gsl-lite
	GIT_TAG e1c381746c2625a76227255f999ae9f14a062208
	DOWNLOAD_ONLY true)
	
if( NOT ${gsl_lite_ADDED} )
	message( FATAL_ERROR "Could not find gsl_lite" )
endif()

####

CPMAddPackage(
	NAME backward_cpp
	GIT_TAG b248998ff393d5e61bec5e45228fa71897aff984
	GITHUB_REPOSITORY bombela/backward-cpp
	OPTIONS
		"BACKWARD_SHARED Off"
		"BACKWARD_TESTS Off"
	DOWNLOAD_ONLY true)
	
if( NOT ${backward_cpp_ADDED} )
	message( FATAL_ERROR "Could not find backward_cpp" )
endif()
	
####

#CPMAddPackage(
#	NAME foonathan_memory
#	GIT_TAG 5f1dfd38f1fa68042962cd71fb79747648df1e74
#	GITHUB_REPOSITORY foonathan/memory
#	DOWNLOAD_ONLY true)
#	
#if( NOT ${foonathan_memory_ADDED} )
#	message( FATAL_ERROR "Could not find foonathan_memory" )
#endif()

####

CPMAddPackage(
	NAME spdlog
	GITHUB_REPOSITORY gabime/spdlog
	GIT_TAG 18e3f07f7d333d33c0ef8fbea02299dbe5d49f97 
	DOWNLOAD_ONLY true)

if( NOT ${spdlog_ADDED} )
	message( FATAL_ERROR "Could not find spdlog" )
endif()

####

CPMAddPackage(
	NAME fmt
	GITHUB_REPOSITORY fmtlib/fmt
	GIT_TAG af283059612dd06c321dc509255ae9b003ca5e69
	DOWNLOAD_ONLY true)

if( NOT ${fmt_ADDED} )
	message( FATAL_ERROR "Could not find fmt" )
endif()

####

CPMAddPackage(
	NAME nfd
	GITHUB_REPOSITORY loopunit/nfd
	GIT_TAG 25812c53ef77f7ac7d40775c0cd5c3446eee4423
	DOWNLOAD_ONLY true)

if( NOT ${nfd_ADDED} )
	message( FATAL_ERROR "Could not find nfd" )
endif()

####

CPMAddPackage(
	NAME boxer
	GITHUB_REPOSITORY aaronmjacobs/Boxer
	GIT_TAG 01c17f6a9cd66068f7890ea887ab3b9a673f0434
	DOWNLOAD_ONLY true)

if( NOT ${boxer_ADDED} )
	message( FATAL_ERROR "Could not find boxer" )
endif()

####

#set(FOONATHAN_MEMORY_BUILD_EXAMPLES OFF)
#set(FOONATHAN_MEMORY_BUILD_TESTS OFF)
#set(FOONATHAN_MEMORY_BUILD_TOOLS OFF)
#add_subdirectory(${foonathan_memory_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/foonathan_memory)

####

set(mu_stdlib_SOURCE_ROOT ${CMAKE_CURRENT_LIST_DIR})

file(GLOB mu_stdlib_headers
	"${mu_stdlib_SOURCE_ROOT}/include/*.h")

file(GLOB mu_stdlib_sources
	"${mu_stdlib_SOURCE_ROOT}/src/*.cpp"
	"${mu_stdlib_SOURCE_ROOT}/src/*.h"
	"${spdlog_SOURCE_DIR}/src/*.cpp"
	"${fmt_SOURCE_DIR}/src/*.cc"
	"${nfd_SOURCE_DIR}/src/nfd_win.cpp"
	"${nfd_SOURCE_DIR}/src/common.h"
	"${nfd_SOURCE_DIR}/src/nfd_common.h"
	"${nfd_SOURCE_DIR}/src/nfd_common.c")

if(APPLE)
	file(GLOB mu_stdlib_platform_sources
		"${boxer_SOURCE_DIR}/src/boxer_osx.mm")
elseif (WIN32)		
	file(GLOB mu_stdlib_platform_sources
		"${boxer_SOURCE_DIR}/src/boxer_win.cpp")
elseif (LINUX)		
	file(GLOB mu_stdlib_platform_sources
		"${boxer_SOURCE_DIR}/src/boxer_linux.cpp")
endif()		

add_library(mu_stdlib STATIC ${mu_stdlib_sources} ${mu_stdlib_headers} ${mu_stdlib_platform_sources})

target_include_directories(mu_stdlib
	PUBLIC 
		$<BUILD_INTERFACE:${backward_cpp_SOURCE_DIR}>
		$<BUILD_INTERFACE:${mu_stdlib_SOURCE_ROOT}/include>
		$<BUILD_INTERFACE:${spdlog_SOURCE_DIR}/include>
		$<BUILD_INTERFACE:${fmt_SOURCE_DIR}/include>
		$<BUILD_INTERFACE:${nfd_SOURCE_DIR}/src>
		$<BUILD_INTERFACE:${nfd_SOURCE_DIR}/src/include>
		$<BUILD_INTERFACE:${boxer_SOURCE_DIR}/include>
		$<BUILD_INTERFACE:${gsl_lite_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>)

target_link_libraries(mu_stdlib
	PUBLIC
		cpm_install::boostorg_leaf
		cpm_install::cwalk
		cpm_install::libuv
		cpm_install::taskflow)

if (WIN32)		
target_link_libraries(mu_stdlib
	PUBLIC
		Winmm.lib)
endif()
set_target_properties(mu_stdlib PROPERTIES CXX_STANDARD 20)

target_compile_definitions(mu_stdlib PUBLIC SPDLOG_COMPILED_LIB SPDLOG_FMT_EXTERNAL)

packageProject(
	NAME mu_stdlib
	VERSION ${PROJECT_VERSION}
	BINARY_DIR ${PROJECT_BINARY_DIR}
	INCLUDE_DIR ${mu_stdlib_SOURCE_ROOT}/include
	INCLUDE_DESTINATION include)

#install(
#	FILES ${backward_cpp_SOURCE_DIR}/backward.hpp
#	DESTINATION include)

install(
	DIRECTORY ${spdlog_SOURCE_DIR}/include/spdlog
	DESTINATION include)

install(
	DIRECTORY ${fmt_SOURCE_DIR}/include/fmt
	DESTINATION include)

install(
	DIRECTORY ${gsl_lite_SOURCE_DIR}/include/gsl
	DESTINATION include)

#install(
#	FILES ${nfd_SOURCE_DIR}/src/include/nfd.h
#	DESTINATION include)
		
if(MSVC)
  if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set_target_properties(mu_stdlib PROPERTIES IMPORTED_CONFIGURATIONS "Debug")
    set_target_properties(mu_stdlib PROPERTIES COMPILE_PDB_NAME mu_stdlib COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib" )
  elseif("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    set_target_properties(mu_stdlib PROPERTIES IMPORTED_CONFIGURATIONS "RelWithDebInfo")
    set_target_properties(mu_stdlib PROPERTIES COMPILE_PDB_NAME mu_stdlib COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib" )
  endif()

  # For example: leaf::result<void> must return { }, otherwise the error will only manifest at runtime.
  # Incorrect handling of this nature will result in "not all control paths return a value", and upgrading 
  # this to an error lets us catch this at compile time.
  target_compile_options(mu_stdlib PUBLIC /we4715)
endif()

if (MU_STDLIB_BUILD_TESTS)
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

		set_target_properties(${arg_TARGET_NAME} PROPERTIES CXX_STANDARD 20)

		target_include_directories(${arg_TARGET_NAME}
			PRIVATE 
				${mu_stdlib_SOURCE_ROOT}/tests)

		target_link_libraries(${arg_TARGET_NAME} 
			PUBLIC
				mu_stdlib)
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
