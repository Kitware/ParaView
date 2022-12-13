/*=========================================================================

  Plugin:   DigitalSignalProcessing
  Module:   vtkAccousticUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
