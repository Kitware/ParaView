// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "Initializer.h"

#include <iostream>
#include <memory>

/// We cannot guarantee that static initializers will be invoked on
/// all platforms, so instead we provide a function (run_me) that
/// initializes the variable.
///
/// In practice, this variable is usually initialized with a value
/// provided by a third-party library as a result of registering
/// some new functionality that will affect the user interface
/// through code outside the plugin.
static std::shared_ptr<int> example;

void run_me()
{
  std::cout << "Running dynamic initializer for \"example\"\n";
  example = std::make_shared<int>(42);
}
