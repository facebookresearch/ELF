#include <algorithm>
#include "class_tests.h"
#include "obj_tests.h"
#include "interop_tests.h"
#include "metatable_tests.h"
#include "reference_tests.h"
#include "selector_tests.h"
#include "error_tests.h"
#include "exception_tests.h"
#include <map>

// A very simple testing framework
// To add a test, author a function with the Test function signature
// and include it in the Map of tests below.
using Test = bool(*)(sel::State &);
using TestMap = std::map<const char *, Test>;
static TestMap tests = {
    {"test_load_error", test_load_error},
    {"test_load_syntax_error", test_load_syntax_error},
    {"test_do_syntax_error", test_do_syntax_error},
    {"test_call_undefined_function", test_call_undefined_function},
    {"test_call_undefined_function2", test_call_undefined_function2},
    {"test_call_stackoverflow", test_call_stackoverflow},
    {"test_parameter_conversion_error", test_parameter_conversion_error},

    {"test_catch_exception_from_callback_within_lua", test_catch_exception_from_callback_within_lua},
    {"test_catch_unknwon_exception_from_callback_within_lua", test_catch_unknwon_exception_from_callback_within_lua},
    {"test_call_exception_handler_for_exception_from_lua", test_call_exception_handler_for_exception_from_lua},
    {"test_call_exception_handler_for_exception_from_callback", test_call_exception_handler_for_exception_from_callback},
    {"test_call_exception_handler_while_using_sel_function", test_call_exception_handler_while_using_sel_function},
    {"test_rethrow_exception_for_exception_from_callback", test_rethrow_exception_for_exception_from_callback},
    {"test_rethrow_using_sel_function", test_rethrow_using_sel_function},
    {"test_throw_on_exception_using_Load", test_throw_on_exception_using_Load},

    {"test_function_no_args", test_function_no_args},
    {"test_add", test_add},
    {"test_multi_return", test_multi_return},
    {"test_multi_return_invoked_once", test_multi_return_invoked_once},
    {"test_heterogeneous_return", test_heterogeneous_return},
    {"test_call_field", test_call_field},
    {"test_call_c_function", test_call_c_function},
    {"test_call_c_fun_from_lua", test_call_c_fun_from_lua},
    {"test_no_return", test_no_return},
    {"test_call_std_fun", test_call_lambda},
    {"test_call_lambda", test_call_lambda},
    {"test_call_normal_c_fun", test_call_normal_c_fun},
    {"test_call_normal_c_fun_many_times", test_call_normal_c_fun_many_times},
    {"test_call_functor", test_call_functor},
    {"test_multivalue_c_fun_return", test_multivalue_c_fun_return},
    {"test_multivalue_c_fun_from_lua", test_multivalue_c_fun_from_lua},
    {"test_embedded_nulls", test_embedded_nulls},
    {"test_coroutine", test_coroutine},
    {"test_pointer_return", test_pointer_return},
    {"test_reference_return", test_reference_return},
    {"test_return_value", test_return_value},
    {"test_return_unregistered_type", test_return_unregistered_type},
    {"test_value_parameter", test_value_parameter},
    {"test_wrong_value_parameter", test_wrong_value_parameter},
    {"test_value_parameter_keeps_type_info", test_value_parameter_keeps_type_info},
    {"test_callback_with_value", test_callback_with_value},
    {"test_nullptr_to_nil", test_nullptr_to_nil},
    {"test_get_primitive_by_value", test_get_primitive_by_value},
    {"test_get_primitive_by_const_ref", test_get_primitive_by_const_ref},
    {"test_get_primitive_by_rvalue_ref", test_get_primitive_by_rvalue_ref},
    {"test_call_with_primitive_by_value", test_call_with_primitive_by_value},
    {"test_call_with_primitive_by_const_ref", test_call_with_primitive_by_const_ref},
    {"test_call_with_primitive_by_rvalue_ref", test_call_with_primitive_by_rvalue_ref},

    {"test_metatable_registry_ptr", test_metatable_registry_ptr},
    {"test_metatable_registry_ref", test_metatable_registry_ref},
    {"test_metatable_ptr_member", test_metatable_ptr_member},
    {"test_metatable_ref_member", test_metatable_ptr_member},

    {"test_register_obj", test_register_obj},
    {"test_register_obj_member_variable", test_register_obj_member_variable},
    {"test_register_obj_to_table", test_register_obj_to_table},
    {"test_mutate_instance", test_mutate_instance},
    {"test_multiple_methods", test_multiple_methods},
    {"test_register_obj_const_member_variable", test_register_obj_const_member_variable},
    {"test_bind_vector_push_back", test_bind_vector_push_back},
    {"test_bind_vector_push_back_string", test_bind_vector_push_back_string},
    {"test_bind_vector_push_back_foos", test_bind_vector_push_back_foos},
    {"test_obj_member_return_pointer", test_obj_member_return_pointer},
    {"test_obj_member_return_ref", test_obj_member_return_ref},
    {"test_obj_member_return_val", test_obj_member_return_val},
    {"test_obj_member_wrong_type", test_obj_member_wrong_type},

    {"test_select_global", test_select_global},
    {"test_select_field", test_select_field},
    {"test_select_index", test_select_index},
    {"test_select_nested_field", test_select_nested_field},
    {"test_select_nested_index", test_select_nested_index},
    {"test_select_equality", test_select_equality},
    {"test_select_cast", test_select_cast},
    {"test_set_global", test_set_global},
    {"test_set_field", test_set_field},
    {"test_set_index", test_set_index},
    {"test_set_nested_field", test_set_nested_field},
    {"test_set_nested_index", test_set_nested_index},
    {"test_create_table_field", test_create_table_field},
    {"test_create_table_index", test_create_table_index},
    {"test_cache_selector_field_assignment", test_cache_selector_field_assignment},
    {"test_cache_selector_field_access", test_cache_selector_field_access},
    {"test_cache_selector_function", test_cache_selector_function},
    {"test_function_should_run_once", test_function_should_run_once},
    {"test_function_result_is_alive_ptr", test_function_result_is_alive_ptr},
    {"test_function_result_is_alive_ref", test_function_result_is_alive_ref},
    {"test_get_and_set_Reference_keeps_identity", test_get_and_set_Reference_keeps_identity},
    {"test_get_and_set_Pointer_keeps_identity", test_get_and_set_Pointer_keeps_identity},
    {"test_selector_call_with_registered_class", test_selector_call_with_registered_class},
    {"test_selector_call_with_registered_class_ptr", test_selector_call_with_registered_class_ptr},
    {"test_selector_call_with_wrong_type_ptr", test_selector_call_with_wrong_type_ptr},
    {"test_selector_call_with_wrong_type_ref", test_selector_call_with_wrong_type_ref},
    {"test_selector_call_with_nullptr_ref", test_selector_call_with_nullptr_ref},
    {"test_selector_get_nullptr_ref", test_selector_get_nullptr_ref},
    {"test_selector_get_wrong_ref", test_selector_get_wrong_ref},
    {"test_selector_get_wrong_ref_to_string", test_selector_get_wrong_ref_to_string},
    {"test_selector_get_wrong_ref_to_table", test_selector_get_wrong_ref_to_table},
    {"test_selector_get_wrong_ref_to_unregistered", test_selector_get_wrong_ref_to_unregistered},
    {"test_selector_get_wrong_ptr", test_selector_get_wrong_ptr},

    {"test_register_class", test_register_class},
    {"test_get_member_variable", test_get_member_variable},
    {"test_set_member_variable", test_set_member_variable},
    {"test_class_field_set", test_class_field_set},
    {"test_class_gc", test_class_gc},
    {"test_ctor_wrong_type", test_ctor_wrong_type},
    {"test_pass_wrong_type", test_pass_wrong_type},
    {"test_pass_pointer", test_pass_pointer},
    {"test_pass_ref", test_pass_ref},
    {"test_return_pointer", test_return_pointer},
    {"test_return_ref", test_return_ref},
    {"test_return_val", test_return_val},
    {"test_freestanding_fun_ref", test_freestanding_fun_ref},
    {"test_freestanding_fun_ptr", test_freestanding_fun_ptr},
    {"test_const_member_function", test_const_member_function},
    {"test_const_member_variable", test_const_member_variable},

    {"test_function_reference", test_function_reference},
    {"test_function_in_constructor", test_function_in_constructor},
    {"test_pass_function_to_lua", test_pass_function_to_lua},
    {"test_call_returned_lua_function", test_call_returned_lua_function},
    {"test_call_multivalue_lua_function", test_call_multivalue_lua_function},
    {"test_call_result_is_alive_ptr", test_call_result_is_alive_ptr},
    {"test_call_result_is_alive_ref", test_call_result_is_alive_ref},
    {"test_function_call_with_registered_class", test_function_call_with_registered_class},
    {"test_function_call_with_registered_class_ptr", test_function_call_with_registered_class_ptr},
    {"test_function_call_with_registered_class_val", test_function_call_with_registered_class_val},
    {"test_function_call_with_registered_class_val_lifetime", test_function_call_with_registered_class_val_lifetime},
    {"test_function_call_with_nullptr_ref", test_function_call_with_nullptr_ref},
    {"test_function_call_with_wrong_ref", test_function_call_with_wrong_ref},
    {"test_function_call_with_wrong_ptr", test_function_call_with_wrong_ptr},
    {"test_function_get_registered_class_by_value", test_function_get_registered_class_by_value},
};

// Executes all tests and returns the number of failures.
int ExecuteAll() {
    const int num_tests = tests.size();
    std::cout << "Executing " << num_tests << " tests..." << std::endl;
    std::vector<std::string> failures;
    int passing = 0;
    for (auto it = tests.begin(); it != tests.end(); ++it) {
        sel::State state{true};
        char progress_marker = 'E';
        bool leaked = false;
        try {
            const bool result = it->second(state);
            int const leak_count = state.Size();
            leaked = leak_count != 0;
            if (result) {
                if (!leaked) {
                    passing += 1;
                    progress_marker = '.';
                } else {
                    failures.push_back(std::string{"Test \""} + it->first
                                       + "\" leaked " + std::to_string(leak_count) + " values");
                    progress_marker = 'l';
                }
            } else {
                progress_marker = 'x';
                failures.push_back(std::string{"Test \""} +
                                   it->first + "\" failed.");
            }
        } catch(std::exception & e) {
            progress_marker = 'e';
            failures.push_back(std::string{"Test \""} + it->first + "\" raised an exception: \"" + e.what() + "\".");
        } catch(...) {
            progress_marker = 'e';
            failures.push_back(std::string{"Test \""} + it->first + "\" raised an exception of unknown type.");
        }
        std::cout << progress_marker << std::flush;
        if (leaked) {
            std::cout << state << std::endl;
        }
    }
    std::cout << std::endl << passing << " out of "
              << num_tests << " tests finished successfully." << std::endl;
    std::cout << std::endl;
    for_each(failures.begin(), failures.end(),
             [](std::string failure) {
                 std::cout << failure << std::endl;
             });
    return num_tests - passing;
}

// Not used in general runs. For debugging purposes
bool ExecuteTest(const char *test) {
    sel::State state{true};
    auto it = tests.find(test);
    const bool result = it->second(state);
    const std::string pretty_result = result ? "pass!" : "fail.";
    std::cout << "Ran test " << test << " with result: "
              << pretty_result << std::endl;
    return result;
}


int main() {
    // Executing all tests will run all test cases and check leftover
    // stack size afterwards. It is expected that the stack size
    // post-test is 0.
    return ExecuteAll();

    // For debugging anything in particular, you can run an individual
    //test like so:
    //ExecuteTest("test_function_should_run_once");
}
