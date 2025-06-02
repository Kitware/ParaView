# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-License-Identifier: BSD-3-Clause
# Test accepting loading
set(on_load_check_callback_accept_test_name TestOnLoadCheckAccept)
set(${on_load_check_callback_accept_test_name}_USES_DIRECT_DATA ON)
paraview_add_client_tests(
    LOAD_PLUGIN OnLoadCheckCallback
    PLUGIN_PATH $<TARGET_FILE_DIR:OnLoadCheckCallback>
    TEST_SCRIPTS ${on_load_check_callback_accept_test_name}.xml)
# Set environment variable for XML test
set_tests_properties(pv.${on_load_check_callback_accept_test_name} PROPERTIES ENVIRONMENT "CTEST_ON_LOAD_CHECK_TESTING=1")

# Test rejecting loading
set(on_load_check_callback_reject_test_name TestOnLoadCheckReject)
set(${on_load_check_callback_reject_test_name}_USES_DIRECT_DATA ON)
paraview_add_client_tests(
    LOAD_PLUGIN OnLoadCheckCallback
    PLUGIN_PATH $<TARGET_FILE_DIR:OnLoadCheckCallback>
    TEST_SCRIPTS ${on_load_check_callback_reject_test_name}.xml)
# Set environment variable for XML test
set_tests_properties(pv.${on_load_check_callback_reject_test_name} PROPERTIES ENVIRONMENT "CTEST_ON_LOAD_CHECK_TESTING=0")
