enable_testing()

if(NOT Boost_USE_STATIC_LIBS)
  add_definitions(-DBOOST_TEST_DYN_LINK)
endif()

set(tests
  "test_asio"
  "test_cq"
  "test_threads"
  "test_tss"
  "test_future"
  "test_mutex"
  "test_tcp_stream"
)

macro(add_test_target target)
  add_executable("${target}" "${target}.cpp")

  set_property(TARGET "${target}" PROPERTY CXX_STANDARD 11)
  set_property(TARGET "${target}" PROPERTY CXX_STANDARD_REQUIRED ON)

  target_link_libraries("${target}"
    boost_green_thread
    ${Boost_CHRONO_LIBRARY}
    ${Boost_CONTEXT_LIBRARY}
    ${Boost_COROUTINE_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

  add_test(NAME "${target}" COMMAND $<TARGET_FILE:${target}>)
endmacro()

foreach(test ${tests})
  add_test_target("${test}")
endforeach()
