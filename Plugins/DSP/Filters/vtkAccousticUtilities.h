// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAccousticUtilities
 * @brief   Defines acoustics constants
 */

#ifndef vtkAccousticUtilities_h
#define vtkAccousticUtilities_h

namespace vtkAccousticUtilities
{
// Reference sound pressure (in Pa) and power (in W) (lowest audible sound for human ears)
static constexpr double REF_PRESSURE = 2.0e-5;
static constexpr double REF_POWER = 1.0e-12;
}

#endif
