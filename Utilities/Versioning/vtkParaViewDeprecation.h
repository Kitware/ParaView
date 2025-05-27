// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#ifndef vtkParaViewDeprecation_h
#define vtkParaViewDeprecation_h

#include "vtkPVVersionQuick.h"

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
// PARAVIEW_DEPRECATED_IN_X_Y_Z("reason for the deprecation")
// void oldApi();
// ```
//
// When selecting which version to deprecate an API in, use the newest macro
// available in this header.
//
// In the implementation:
//
// ```cxx
// // Hide PARAVIEW_DEPRECATED_IN_X_Y_Z() warnings for this class.
// #define PARAVIEW_DEPRECATION_LEVEL 0
//
// #include "vtkLegacy.h"
//
// void oldApi()
// {
//   // One of:
//   VTK_LEGACY_BODY(oldApi, "ParaView X.Y.Z");
//   VTK_LEGACY_REPLACED_BODY(oldApi, "ParaView X.Y.Z", newApi);
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
#ifdef PARAVIEW_VERSION_NUMBER
#define PARAVIEW_DEPRECATION_LEVEL PARAVIEW_VERSION_NUMBER
#else
#define PARAVIEW_DEPRECATION_LEVEL PARAVIEW_VERSION_NUMBER_QUICK
#endif
#endif

// API deprecated before 5.11.0 have already been removed.
#define PARAVIEW_MINIMUM_DEPRECATION_LEVEL PARAVIEW_VERSION_CHECK(5, 11, 0)

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

// APIs deprecated in 6.0.0.
#if defined(__VTK_WRAP__)
#define PARAVIEW_DEPRECATED_IN_6_0_0(reason) [[vtk::deprecated(reason, "6.0.0")]]
#elif PARAVIEW_DEPRECATION_LEVEL >= PARAVIEW_VERSION_CHECK(5, 13, 20240101)
#define PARAVIEW_DEPRECATED_IN_6_0_0(reason) PARAVIEW_DEPRECATION(reason)
#else
#define PARAVIEW_DEPRECATED_IN_6_0_0(reason)
#endif

// APIs deprecated in 5.13.0.
#if defined(__VTK_WRAP__)
#define PARAVIEW_DEPRECATED_IN_5_13_0(reason) [[vtk::deprecated(reason, "5.13.0")]]
#elif PARAVIEW_DEPRECATION_LEVEL >= PARAVIEW_VERSION_CHECK(5, 12, 20230101)
#define PARAVIEW_DEPRECATED_IN_5_13_0(reason) PARAVIEW_DEPRECATION(reason)
#else
#define PARAVIEW_DEPRECATED_IN_5_13_0(reason)
#endif

// APIs deprecated in the older release always warn.
#if defined(__VTK_WRAP__)
#define PARAVIEW_DEPRECATED_IN_5_12_0(reason) [[vtk::deprecated(reason, "5.12.0")]]
#define PARAVIEW_DEPRECATED_IN_5_11_0(reason) [[vtk::deprecated(reason, "5.11.0")]]
#define PARAVIEW_DEPRECATED_IN_5_10_0(reason) [[vtk::deprecated(reason, "5.10.0")]]
#else
#define PARAVIEW_DEPRECATED_IN_5_12_0(reason) PARAVIEW_DEPRECATION(reason)
#define PARAVIEW_DEPRECATED_IN_5_11_0(reason) PARAVIEW_DEPRECATION(reason)
#define PARAVIEW_DEPRECATED_IN_5_10_0(reason) PARAVIEW_DEPRECATION(reason)
#endif

#endif

// VTK-HeaderTest-Exclude: vtkParaViewDeprecation.h
