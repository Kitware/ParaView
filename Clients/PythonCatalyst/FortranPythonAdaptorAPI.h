// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef FortranPythonAdaptorAPI_h
#define FortranPythonAdaptorAPI_h

#include "FortranPythonAdaptorAPIMangling.h"
#include "vtkPVPythonCatalystModule.h"

// This code is meant to be used as an API for C simulation
// codes. To use with C codes, include this header file. Call the
// 'extern "C"' functions as named below for both Fortran and C.
// C code should include this header file to get the properly
// mangled function names.

#include "CPythonAdaptorAPI.h"

#endif
