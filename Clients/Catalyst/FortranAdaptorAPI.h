// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef FortranAdaptorAPI_h
#define FortranAdaptorAPI_h

#include "FortranAdaptorAPIMangling.h"
#include "vtkPVCatalystModule.h"

// This code is meant to be used as an API for C simulation
// codes. To use with C codes, include this header file. Call the
// 'extern "C"' functions as named below for both Fortran and C.
// C code should include this header file to get the properly
// mangled function names.

#include "CAdaptorAPI.h"

#endif
