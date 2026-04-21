// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSessionIterator
 *
 * vtkSessionIterator is used to iterate over sessions in the global
 * ProcessModule.
 */

#ifndef vtkSessionIterator_h
#define vtkSessionIterator_h

#include "vtkPVSessionIterator.h"
#include "vtkParaViewDeprecation.h" // for PARAVIEW_DEPRECATED

PARAVIEW_DEPRECATED_IN_6_1_0("Please use the `vtkPVSessionIterator` class instead.")
typedef vtkPVSessionIterator vtkSessionIterator;

#endif
