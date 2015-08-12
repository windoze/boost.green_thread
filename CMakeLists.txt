cmake_minimum_required(VERSION 3.1.0)
project(Boost.Green_Thread)

find_package(Threads REQUIRED)
find_package(Boost 1.56.0 COMPONENTS
        chrono
	context
        coroutine
        system
        thread
        unit_test_framework
        REQUIRED)

# Configure options
option(BUILD_TESTS
  "Build tests" YES
)

option(BUILD_EXAMPLES
  "Build examples" YES
)

# Install info
set(includedir "include")
set(libdir "lib")

# Targets
add_subdirectory(include)

# src
set(library_SRC
	src/condition.cpp
	src/future.cpp
	src/mutex.cpp
	src/scheduler_object.cpp
	src/scheduler_object.hpp
	src/thread_object.cpp
	src/thread_object.hpp)
set(library_HDR
	include/boost/green_thread.hpp
	include/boost/green_thread/asio/detail/use_future.hpp
	include/boost/green_thread/asio/detail/yield.hpp
	include/boost/green_thread/asio/use_future.hpp
	include/boost/green_thread/asio/yield.hpp
	include/boost/green_thread/asio.hpp
	include/boost/green_thread/barrier.hpp
	include/boost/green_thread/concurrent_queue.hpp
	include/boost/green_thread/condition_variable.hpp
	include/boost/green_thread/detail/config.hpp
	include/boost/green_thread/detail/thread_base.hpp
	include/boost/green_thread/detail/thread_data.hpp
	include/boost/green_thread/detail/forward.hpp
	include/boost/green_thread/detail/spinlock.hpp
	include/boost/green_thread/detail/std_stream_guard.hpp
	include/boost/green_thread/detail/utility.hpp
	include/boost/green_thread/exceptions.hpp
	include/boost/green_thread/thread.hpp
	include/boost/green_thread/thread_only.hpp
	include/boost/green_thread/thread_group.hpp
        include/boost/green_thread/greenify.hpp
        include/boost/green_thread/tss.hpp
	include/boost/green_thread/future/async.hpp
	include/boost/green_thread/future/detail/shared_state.hpp
	include/boost/green_thread/future/detail/shared_state_object.hpp
	include/boost/green_thread/future/detail/task_base.hpp
	include/boost/green_thread/future/detail/task_object.hpp
	include/boost/green_thread/future/future.hpp
	include/boost/green_thread/future/future_status.hpp
	include/boost/green_thread/future/packaged_task.hpp
	include/boost/green_thread/future/promise.hpp
	include/boost/green_thread/future.hpp
	include/boost/green_thread/iostream.hpp
	include/boost/green_thread/mutex.hpp
	include/boost/green_thread/shared_mutex.hpp
        include/boost/green_thread/streambuf.hpp
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include" ${Boost_INCLUDE_DIR})

add_library("boost_green_thread" ${library_SRC} ${library_HDR})

set_property(TARGET "boost_green_thread" PROPERTY CXX_STANDARD 11)
set_property(TARGET "boost_green_thread" PROPERTY CXX_STANDARD_REQUIRED ON)

target_compile_definitions("boost_green_thread"
	PRIVATE BOOST_GREEN_THREAD_SOURCE)

if (BUILD_SHARED_LIBS)
	target_compile_definitions("boost_green_thread"
		PRIVATE BOOST_GREEN_THREAD_DYN_LINK)
endif()

target_link_libraries("boost_green_thread"
  ${Boost_CHRONO_LIBRARY}
  ${Boost_CONTEXT_LIBRARY}
  ${Boost_COROUTINE_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_THREAD_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT})

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

if(BUILD_EXAMPLES)
  add_subdirectory(example)
endif()

install(TARGETS "boost_green_thread"
  DESTINATION "${libdir}"
)

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

# CTest
if(BUILD_TESTS)
	include(CTest)
endif()

