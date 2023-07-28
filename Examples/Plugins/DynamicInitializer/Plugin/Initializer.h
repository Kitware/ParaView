// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   Initializer
 *
 * This header demonstrates how to guarantee the initialization of a
 * static variable (in Initializer.cxx) upon plugin load, even when
 * an operating system performs lazy evaluation of static variables
 * (i.e., avoids initialization until they are first referenced). To
 * ensure the variable is initialized, we have the plugin call a
 * function to dynamically initialize the variable.
 */
#ifndef PluginDynamicInitializer_h
#define PluginDynamicInitializer_h

void run_me();

#endif
