cmake_minimum_required(VERSION 3.15)

project(CVM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(cvmpp
    src/main.cpp
    src/lexer/lexer.cpp
    src/parser/parser.cpp
    src/vm/compiler.cpp
    src/vm/vm.cpp
)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME parser_arrays COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/parser_arrays.pi --parse-only)
    add_test(NAME array_declarations_runtime COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/array_declarations.pi)
    set_tests_properties(array_declarations_runtime PROPERTIES PASS_REGULAR_EXPRESSION "\\[A, B, C\\]")
    add_test(NAME java_types_runtime COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/java_types.pi)
    set_tests_properties(java_types_runtime PROPERTIES PASS_REGULAR_EXPRESSION "answer=43;3")
    add_test(NAME legacy_type_rejected COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/legacy_type.pi)
    set_tests_properties(legacy_type_rejected PROPERTIES WILL_FAIL TRUE)
    add_test(NAME incompatible_assignment_rejected COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/type_error.pi)
    set_tests_properties(incompatible_assignment_rejected PROPERTIES WILL_FAIL TRUE)
endif()
