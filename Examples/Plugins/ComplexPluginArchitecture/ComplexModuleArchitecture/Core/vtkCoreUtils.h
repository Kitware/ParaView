/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCoreUtils.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
/**
 * @class   vtkCoreUtils
 * @brief   provide core utils
 */

#ifndef vtkCoreUtils_h
#define vtkCoreUtils_h

#include "CoreModule.h"

#include <vtkObject.h>

class CORE_EXPORT vtkCoreUtils : public vtkObject
{
public:
  static vtkCoreUtils* New();
  vtkTypeMacro(vtkCoreUtils, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Convert degrees into radians
   */
  static float RadiansFromDegrees(float degrees);
  static double RadiansFromDegrees(double degrees);
  //@}
};
#endif
