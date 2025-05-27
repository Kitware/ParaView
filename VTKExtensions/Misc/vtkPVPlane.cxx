// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

// Hide PARAVIEW_DEPRECATED_IN_6_0_0() warnings for this class.
#define PARAVIEW_DEPRECATION_LEVEL 0

#include "vtkPVPlane.h"

#include "vtkObjectFactory.h"

#include <cmath>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPlane);

//----------------------------------------------------------------------------
vtkPVPlane::vtkPVPlane() = default;

//----------------------------------------------------------------------------
vtkPVPlane::~vtkPVPlane() = default;
