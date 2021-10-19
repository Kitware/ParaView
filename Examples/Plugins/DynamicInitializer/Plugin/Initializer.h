/*=========================================================================

  Program:   ParaView
  Module:    Initializer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
