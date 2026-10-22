# SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(calculator-engine-tests
    main.cpp
    calculator_test.cpp
    decimal_test.cpp
    evaluator_test.cpp
)
target_link_libraries(calculator-engine-tests PRIVATE calculator-engine GTest::gtest)
gtest_discover_tests(calculator-engine-tests)

add_executable(calculator-ui-tests ui_test.cpp)
target_link_libraries(calculator-ui-tests PRIVATE calculator-ui GTest::gtest)
# The application tests group every running copy of this binary.
gtest_discover_tests(calculator-ui-tests PROPERTIES RESOURCE_LOCK calculator-ui-tests)

add_executable(calculator-capture capture.cpp)
target_link_libraries(calculator-capture PRIVATE calculator-ui)
