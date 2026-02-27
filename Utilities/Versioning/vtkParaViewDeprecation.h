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
// #include …
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

// API deprecated before 6.0.0 have already been removed.
#define PARAVIEW_MINIMUM_DEPRECATION_LEVEL PARAVIEW_VERSION_CHECK(6, 0, 0)

// Force the deprecation level to be at least that of ParaView's build
// configuration.
#if PARAVIEW_DEPRECATION_LEVEL < PARAVIEW_MINIMUM_DEPRECATION_LEVEL
#undef PARAVIEW_DEPRECATION_LEVEL
#define PARAVIEW_DEPRECATION_LEVEL PARAVIEW_MINIMUM_DEPRECATION_LEVEL
#endif

// Deprecation macro support for various compilers.
#if defined(VTK_WRAPPING_CXX)
// Ignore deprecation in wrapper code.
#define PARAVIEW_DEPRECATION(reason)
#elif defined(__VTK_WRAP__)
#define PARAVIEW_DEPRECATION(reason) [[vtk::deprecated(reason)]]
#else
#if defined(__clang__)
// Clang 12 and AppleClang 13 and before mix [[deprecated]] with visibility macros, and cause parser
// like below error: expected identifier before '__attribute__' class [[deprecated("deprecated")]]
// __attribute__((visibility("default"))) Foo {};
#if (defined(__apple_build_version__) && (__clang_major__ <= 13))
#define VTK_DEPRECATION(reason) __attribute__((__deprecated__(reason)))
#elif (__clang_major__ <= 12)
#define PARAVIEW_DEPRECATION(reason) __attribute__((__deprecated__(reason)))
#else
#define PARAVIEW_DEPRECATION(reason) [[deprecated(reason)]]
#endif
#elif defined(__GNUC__)
// GCC 12 and before mix [[deprecated]] with visibility macros, and cause parser like below
// error: expected identifier before '__attribute__'
// class [[deprecated("deprecated")]] __attribute__((visibility("default"))) Foo {};
#if (__GNUC__ <= 12)
#define PARAVIEW_DEPRECATION(reason) __attribute__((__deprecated__(reason)))
#else
#define PARAVIEW_DEPRECATION(reason) [[deprecated(reason)]]
#endif
#else
#define PARAVIEW_DEPRECATION(reason) [[deprecated(reason)]]
#endif
#endif

// APIs deprecated in the next release.
#if defined(__VTK_WRAP__)
#define PARAVIEW_DEPRECATED_IN_6_2_0(reason) [[vtk::deprecated(reason, "6.2.0")]]
#elif PARAVIEW_DEPRECATION_LEVEL >= PARAVIEW_VERSION_CHECK(6, 1, 20260123)
#define PARAVIEW_DEPRECATED_IN_6_2_0(reason) PARAVIEW_DEPRECATION(reason)
#else
#define PARAVIEW_DEPRECATED_IN_6_2_0(reason)
#endif

// APIs deprecated in 6.1.0.
#if defined(__VTK_WRAP__)
#define PARAVIEW_DEPRECATED_IN_6_1_0(reason) [[vtk::deprecated(reason, "6.1.0")]]
#elif PARAVIEW_DEPRECATION_LEVEL >= PARAVIEW_VERSION_CHECK(6, 0, 20250520)
#define PARAVIEW_DEPRECATED_IN_6_1_0(reason) PARAVIEW_DEPRECATION(reason)
#else
#define PARAVIEW_DEPRECATED_IN_6_1_0(reason)
#endif

// APIs deprecated in the older release always warn.
#if defined(__VTK_WRAP__)
#define PARAVIEW_DEPRECATED_IN_6_0_0(reason) [[vtk::deprecated(reason, "6.0.0")]]
#define PARAVIEW_DEPRECATED_IN_5_13_0(reason) [[vtk::deprecated(reason, "5.13.0")]]
#else
#define PARAVIEW_DEPRECATED_IN_6_0_0(reason) PARAVIEW_DEPRECATION(reason)
#define PARAVIEW_DEPRECATED_IN_5_13_0(reason) PARAVIEW_DEPRECATION(reason)
#endif

#endif

// VTK-HeaderTest-Exclude: vtkParaViewDeprecation.h
