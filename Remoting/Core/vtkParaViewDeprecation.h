/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParaViewDeprecation.h

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkParaViewDeprecation_h
#define vtkParaViewDeprecation_h

#include "vtkPVVersion.h"

//----------------------------------------------------------------------------
// These macros may be used to deprecate APIs in ParaView. They act as
// attributes on method declarations and do not remove methods from a build
// based on build configuration.
//
// To use:
//
// In the declaration:
//
// ```cxx
// PARAVIEW_DEPRECATED_IN_5_10_0("reason for the deprecation")
// void oldApi();
// ```
//
// When selecting which version to deprecate an API in, use the newest macro
// available in this header.
//
// In the implementation:
//
// ```cxx
// // Hide PARAVIEW_DEPRECATED_IN_5_10_0() warnings for this class.
// #define PARAVIEW_DEPRECATION_LEVEL 0
//
// #include "vtkLegacy.h"
//
// void oldApi()
// {
//   // One of:
//   VTK_LEGACY_BODY(oldApi, "ParaView 5.10.0");
//   VTK_LEGACY_REPLACED_BODY(oldApi, "ParaView 5.10.0", newApi);
//
//   // Remaining implementation.
// }
// ```
//
// Please note the `PARAVIEW_DEPRECATED_IN_` version in the
// `PARAVIEW_DEPRECATION_LEVEL` comment so that it can be removed when that
// version is finally removed.
//----------------------------------------------------------------------------

// The level at which warnings should be made.
#ifndef PARAVIEW_DEPRECATION_LEVEL
// ParaView defaults to deprecation of its current version.
#define PARAVIEW_DEPRECATION_LEVEL PARAVIEW_VERSION_NUMBER
#endif

// API deprecated before 5.9.0 have already been removed.
#define PARAVIEW_MINIMUM_DEPRECATION_LEVEL PARAVIEW_VERSION_CHECK(5, 9, 0)

// Force the deprecation level to be at least that of ParaView's build
// configuration.
#if PARAVIEW_DEPRECATION_LEVEL < PARAVIEW_MINIMUM_DEPRECATION_LEVEL
#undef PARAVIEW_DEPRECATION_LEVEL
#define PARAVIEW_DEPRECATION_LEVEL PARAVIEW_MINIMUM_DEPRECATION_LEVEL
#endif

// Deprecation macro support for various compilers.
#if 0 && __cplusplus >= 201402L
// This is currently hard-disabled because compilers do not mix C++ attributes
// and `__attribute__` extensions together well.
#define PARAVIEW_DEPRECATION(reason) [[deprecated(reason)]]
#elif defined(VTK_WRAPPING_CXX)
// Ignore deprecation in wrapper code.
#define PARAVIEW_DEPRECATION(reason)
#elif defined(__VTK_WRAP__)
#define PARAVIEW_DEPRECATION(reason) [[vtk::deprecated(reason)]]
#else
#if defined(_WIN32) || defined(_WIN64)
#define PARAVIEW_DEPRECATION(reason) __declspec(deprecated(reason))
#elif defined(__clang__)
#if __has_extension(attribute_deprecated_with_message)
#define PARAVIEW_DEPRECATION(reason) __attribute__((__deprecated__(reason)))
#else
#define PARAVIEW_DEPRECATION(reason) __attribute__((__deprecated__))
#endif
#elif defined(__GNUC__)
#if (__GNUC__ >= 5) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5))
#define PARAVIEW_DEPRECATION(reason) __attribute__((__deprecated__(reason)))
#else
#define PARAVIEW_DEPRECATION(reason) __attribute__((__deprecated__))
#endif
#else
#define PARAVIEW_DEPRECATION(reason)
#endif
#endif

// APIs deprecated in the next release.
#if defined(__VTK_WRAP__)
#define PARAVIEW_DEPRECATED_IN_5_11_0(reason) [[vtk::deprecated(reason, "5.11.0")]]
#elif PARAVIEW_DEPRECATION_LEVEL >= PARAVIEW_VERSION_CHECK(5, 11, 0)
#define PARAVIEW_DEPRECATED_IN_5_11_0(reason) PARAVIEW_DEPRECATION(reason)
#else
#define PARAVIEW_DEPRECATED_IN_5_11_0(reason)
#endif

// APIs deprecated in 5.10.0.
#if defined(__VTK_WRAP__)
#define PARAVIEW_DEPRECATED_IN_5_10_0(reason) [[vtk::deprecated(reason, "5.10.0")]]
#elif PARAVIEW_DEPRECATION_LEVEL >= PARAVIEW_VERSION_CHECK(5, 10, 0)
#define PARAVIEW_DEPRECATED_IN_5_10_0(reason) PARAVIEW_DEPRECATION(reason)
#else
#define PARAVIEW_DEPRECATED_IN_5_10_0(reason)
#endif

// APIs deprecated in the older release always warn.
#if defined(__VTK_WRAP__)
#define PARAVIEW_DEPRECATED_IN_5_9_0(reason) [[vtk::deprecated(reason, "5.9.0")]]
#else
#define PARAVIEW_DEPRECATED_IN_5_9_0(reason) PARAVIEW_DEPRECATION(reason)
#endif

#endif

// VTK-HeaderTest-Exclude: vtkParaViewDeprecation.h
