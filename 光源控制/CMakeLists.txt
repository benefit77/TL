cmake_minimum_required(VERSION 3.16)

project(libledim
	VERSION 2.0.0
	LANGUAGES C
	DESCRIPTION "light source (LED illuminator) control library"
)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "build type" FORCE)
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# let an editor index the tree properly (vs code, clion, vim with a language server, ...)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	add_compile_options(-Wall -Wextra)
endif()

include(GNUInstallDirs)
include(CTest)

# upstream, in one list: the protocol, the transport, the port walk, the logger, the device, and the
# facade that keeps the first generation interface alive
set(libledim_src_files
	libledim/src/libledim_proto.c
	libledim/src/libledim_log.c
	libledim/src/libledim_portscan.c
	libledim/src/libledim_device.c
	libledim/src/libledim_compat.c
)

# one transport and one port walk per host
if(WIN32)
	list(APPEND libledim_src_files libledim/src/libledim_transport_win32.c)
	list(APPEND libledim_src_files libledim/src/libledim_portscan_win32.c)
else()
	list(APPEND libledim_src_files libledim/src/libledim_transport_posix.c)
	list(APPEND libledim_src_files libledim/src/libledim_portscan_posix.c)
endif()

set(libledim_private_inc_dirs
	${CMAKE_CURRENT_SOURCE_DIR}/libledim/src
)

# the build tree mirrors the delivered layout on purpose: the programs land in the top of the build
# directory and the library in a libledim/ folder next to them, which is exactly how this project is
# laid out once copied somewhere, so what is built can be copied over without any reshuffling
set(LEDIM_LIB_OUTPUT_DIR "${CMAKE_BINARY_DIR}/libledim")
set(LEDIM_BIN_OUTPUT_DIR "${CMAKE_BINARY_DIR}")

add_library(libledim SHARED ${libledim_src_files})
target_include_directories(libledim
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libledim/include
	PRIVATE ${libledim_private_inc_dirs}
)
target_compile_definitions(libledim PRIVATE LIBLEDIM_BUILD_AS_DLL)

add_library(libledim_static STATIC ${libledim_src_files})
target_include_directories(libledim_static
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libledim/include
	PRIVATE ${libledim_private_inc_dirs}
)
target_compile_definitions(libledim_static PUBLIC LIBLEDIM_STATIC)

# cmake prepends the "lib" prefix by default, which would end up as "liblibledim.so"
set_target_properties(libledim PROPERTIES PREFIX "")
set_target_properties(libledim_static PROPERTIES PREFIX "" OUTPUT_NAME "libledim")

set_target_properties(libledim libledim_static PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY ${LEDIM_LIB_OUTPUT_DIR}
	ARCHIVE_OUTPUT_DIRECTORY ${LEDIM_LIB_OUTPUT_DIR}
	RUNTIME_OUTPUT_DIRECTORY ${LEDIM_LIB_OUTPUT_DIR}
)

# the public header is the entire contract: everything else stays out of the dynamic symbol table
set_target_properties(libledim PROPERTIES
	C_VISIBILITY_PRESET hidden
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
)

add_executable(libledim-clidemo 命令行/libledim_cli.c)
target_link_libraries(libledim-clidemo PRIVATE libledim)
# $ORIGIN, so that the two programs find the library next to themselves: in the build tree (where
# libledim.so is also built to) and after the whole thing is copied into a single folder, which is
# how the ready to use package is laid out
set_target_properties(libledim-clidemo PROPERTIES
	BUILD_RPATH "$ORIGIN/libledim"
	INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
	RUNTIME_OUTPUT_DIRECTORY ${LEDIM_BIN_OUTPUT_DIR}
)

# the graphical front end
#
# it needs a widget toolkit, so it is built only when one is around; the library itself has no
# dependency whatsoever. Qt is looked up through its own CMake package, which every Qt install ships.
find_package(Qt5 QUIET COMPONENTS Widgets)
if(Qt5Widgets_FOUND)
	# the library is plain C, only the window is C++; asking for the language here keeps a machine
	# that has a C compiler and no C++ compiler able to build the library on its own
	enable_language(CXX)

	add_executable(libledim-gui
		界面/ledim_gui_main.cpp
		界面/ledim_main_window.cpp
		界面/ledim_channel_column.cpp
		界面/ledim_mode_dialog.cpp
	)
	target_link_libraries(libledim-gui PRIVATE libledim Qt5::Widgets)
	set_target_properties(libledim-gui PROPERTIES
		BUILD_RPATH "$ORIGIN/libledim"
		INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
		RUNTIME_OUTPUT_DIRECTORY ${LEDIM_BIN_OUTPUT_DIR}
	)
	set_property(TARGET libledim-gui PROPERTY AUTOMOC ON)
	set_target_properties(libledim-gui PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
else()
	message(STATUS "libledim-gui is off, no Qt5 Widgets was found (install qtbase5-dev to build it)")
endif()

# --------------------------------------------------------------------------------------------
# installation
# --------------------------------------------------------------------------------------------

install(
	TARGETS libledim libledim_static
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(
	FILES
		libledim/include/libledim.h
		libledim/include/magic.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libledim
)
install(TARGETS libledim-clidemo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(TARGET libledim-gui)
	install(TARGETS libledim-gui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/ledim.pc.in
	${CMAKE_CURRENT_BINARY_DIR}/ledim.pc
	@ONLY
)
install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/ledim.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# --------------------------------------------------------------------------------------------
# tests
# --------------------------------------------------------------------------------------------

if(BUILD_TESTING)
	# the protocol layer has no dependencies at all, so it is tested against fixed vectors, with the
	# source compiled into the test rather than linked out of the library
	add_executable(test_libledim_proto 测试/test_libledim_proto.c libledim/src/libledim_proto.c)
	target_include_directories(test_libledim_proto PRIVATE libledim/src)
	add_test(NAME proto COMMAND test_libledim_proto)

	find_package(Python3 COMPONENTS Interpreter)
	if(Python3_Interpreter_FOUND)
		# the integration test drives the cli against a stand-in for the controller firmware, over a pty
		add_test(
			NAME integration
			COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/测试/run_tests.py
				--cli $<TARGET_FILE:libledim-clidemo>
				--lib-dir $<TARGET_FILE_DIR:libledim>
				--test-dir ${CMAKE_CURRENT_BINARY_DIR}/tests
				--mode integration
		)

		if(TARGET libledim-gui)
			# the window is started without a screen, made to click through one channel, and the
			# frames the stand-in controller saw are checked, so the wiring behind every control is
			# covered by the test run too
			add_test(
				NAME gui
				COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/测试/run_tests.py
					--gui $<TARGET_FILE:libledim-gui>
					--test-dir ${CMAKE_CURRENT_BINARY_DIR}/tests
					--mode gui
			)
		endif()

		# the differential test replays the same program against the first generation of libledim and
		# against this one, and demands that both sent the same bytes and answered the same values.
		# it defaults to the framework based port of the first generation that sits next to this
		# source tree, which is also what makes the two comparable at all; point the two variables
		# somewhere else to compare against another build, or leave them empty to skip the test.
		set(LEDIM_BASELINE_LIB_DIR "" CACHE PATH "directory of a first generation libledim.so to compare against")
		set(LEDIM_BASELINE_INCLUDE_DIR "" CACHE PATH "include directory of a first generation libledim")

		# the framework based port of the first generation may sit next to this source tree (the
		# usual place on the machine this was developed on) or one level further up (when this source
		# tree lives inside the ready to use project folder); anywhere else it is simply absent, and
		# the differential test then bows out with the message below rather than failing
		if(NOT LEDIM_BASELINE_LIB_DIR)
			foreach(_candidate
					"${CMAKE_CURRENT_SOURCE_DIR}/../LEDim"
					"${CMAKE_CURRENT_SOURCE_DIR}/../../LEDim")
				if(EXISTS "${_candidate}/build/projects/libledim/libledim.so")
					set(LEDIM_BASELINE_LIB_DIR "${_candidate}/build/projects/libledim")
					set(LEDIM_BASELINE_INCLUDE_DIR "${_candidate}/projects/libledim/inc/libledim")
					break()
				endif()
			endforeach()
		endif()
		if(EXISTS "${LEDIM_BASELINE_LIB_DIR}/libledim.so" AND EXISTS "${LEDIM_BASELINE_INCLUDE_DIR}/libledim.h")
			add_test(
				NAME differential
				COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/测试/run_tests.py
					--cli $<TARGET_FILE:libledim-clidemo>
					--lib-dir $<TARGET_FILE_DIR:libledim>
					--baseline-lib-dir ${LEDIM_BASELINE_LIB_DIR}
					--baseline-include-dir ${LEDIM_BASELINE_INCLUDE_DIR}
					--test-dir ${CMAKE_CURRENT_BINARY_DIR}/tests
					--mode differential
			)
		else()
			message(STATUS "the differential test is off, no first generation libledim was found at "
				"${LEDIM_BASELINE_LIB_DIR}")
		endif()
	endif()
endif()
