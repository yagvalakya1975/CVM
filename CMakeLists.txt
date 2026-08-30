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
    add_executable(local_slots_bytecode_test
        tests/local_slots_bytecode.cpp
        src/lexer/lexer.cpp
        src/parser/parser.cpp
        src/vm/compiler.cpp
    )
    add_test(NAME local_slots_bytecode COMMAND local_slots_bytecode_test)
    add_executable(char_numeric_bytecode_test
        tests/char_numeric_bytecode.cpp
        src/lexer/lexer.cpp
        src/parser/parser.cpp
        src/vm/compiler.cpp
    )
    add_test(NAME char_numeric_bytecode COMMAND char_numeric_bytecode_test)
    add_executable(array_binary_typecheck_test
        tests/array_binary_typecheck.cpp
        src/lexer/lexer.cpp
        src/parser/parser.cpp
        src/vm/compiler.cpp
    )
    add_test(NAME array_binary_typecheck COMMAND array_binary_typecheck_test)
    add_executable(runtime_value_model_test
        tests/runtime_value_model.cpp
        src/vm/vm.cpp
    )
    add_test(NAME runtime_value_model COMMAND runtime_value_model_test)
    add_test(NAME local_slots_runtime COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/local_slots.pi)
    set_tests_properties(local_slots_runtime PROPERTIES PASS_REGULAR_EXPRESSION "1;10;31;40")
    add_test(NAME parser_arrays COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/parser_arrays.pi --parse-only)
    add_test(NAME array_declarations_runtime COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/array_declarations.pi)
    set_tests_properties(array_declarations_runtime PROPERTIES PASS_REGULAR_EXPRESSION "\\[A, B, C\\]")
    add_test(NAME java_types_runtime COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/java_types.pi)
    set_tests_properties(java_types_runtime PROPERTIES PASS_REGULAR_EXPRESSION "answer=43;3")
    add_test(NAME numeric_conversions_runtime COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/numeric_conversions.pi)
    set_tests_properties(numeric_conversions_runtime PROPERTIES PASS_REGULAR_EXPRESSION "3;3.0;A;3;3;3;A;65;3.0;66;66")
    add_test(NAME lowercase_string_type_rejected COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/legacy_type.pi)
    set_tests_properties(lowercase_string_type_rejected PROPERTIES WILL_FAIL TRUE)
    add_test(NAME incompatible_assignment_rejected COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/type_error.pi)
    set_tests_properties(incompatible_assignment_rejected PROPERTIES WILL_FAIL TRUE)
    add_test(NAME functions_runtime COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/functions.pi)
    set_tests_properties(functions_runtime PROPERTIES PASS_REGULAR_EXPRESSION "5;8")
    add_test(NAME function_void_runtime COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/function_void.pi)
    set_tests_properties(function_void_runtime PROPERTIES PASS_REGULAR_EXPRESSION "hello")
    add_test(NAME function_bad_arg_count COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/function_bad_arg_count.pi)
    set_tests_properties(function_bad_arg_count PROPERTIES WILL_FAIL TRUE)
    add_test(NAME function_forward_call_rejected COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/function_forward_call.pi)
    set_tests_properties(function_forward_call_rejected PROPERTIES WILL_FAIL TRUE)
    add_test(NAME function_bad_arg_type COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/function_bad_arg_type.pi)
    set_tests_properties(function_bad_arg_type PROPERTIES WILL_FAIL TRUE)
    add_test(NAME function_bad_return COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/function_bad_return.pi)
    set_tests_properties(function_bad_return PROPERTIES WILL_FAIL TRUE)
    add_test(NAME function_return_outside COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/function_return_outside.pi)
    set_tests_properties(function_return_outside PROPERTIES WILL_FAIL TRUE)
    add_test(NAME cli_dump_ast COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/local_slots.pi --dump-ast)
    set_tests_properties(cli_dump_ast PROPERTIES PASS_REGULAR_EXPRESSION "AST")
    add_test(NAME cli_dump_bytecode COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/local_slots.pi --dump-bytecode)
    set_tests_properties(cli_dump_bytecode PROPERTIES PASS_REGULAR_EXPRESSION "BYTECODE DISASSEMBLY")
    add_test(NAME cli_tokens_parse_only COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/local_slots.pi --tokens --parse-only)
    set_tests_properties(cli_tokens_parse_only PROPERTIES PASS_REGULAR_EXPRESSION "Token:")
    add_test(NAME cli_old_ast_flag_rejected COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/local_slots.pi --ast)
    set_tests_properties(cli_old_ast_flag_rejected PROPERTIES WILL_FAIL TRUE)
    add_test(NAME cli_old_bytecode_flag_rejected COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/local_slots.pi --bytecode)
    set_tests_properties(cli_old_bytecode_flag_rejected PROPERTIES WILL_FAIL TRUE)
    add_test(NAME cli_multiple_sources_rejected COMMAND cvmpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/local_slots.pi ${CMAKE_CURRENT_SOURCE_DIR}/tests/functions.pi)
    set_tests_properties(cli_multiple_sources_rejected PROPERTIES WILL_FAIL TRUE)
endif()
