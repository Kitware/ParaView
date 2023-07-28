// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCoreUtils
 * @brief   provide core utils
 */

#ifndef vtkCoreUtils_h
#define vtkCoreUtils_h

#include "CoreModule.h" // For export macro

#include <vtkObject.h>

class CORE_EXPORT vtkCoreUtils : public vtkObject
{
public:
  static vtkCoreUtils* New();
  vtkTypeMacro(vtkCoreUtils, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Convert degrees into radians
   */
  static float RadiansFromDegrees(float degrees);
  static double RadiansFromDegrees(double degrees);
  ///@}

  vtkCoreUtils(const vtkCoreUtils&) = delete;
  void operator=(const vtkCoreUtils&) = delete;

protected:
  vtkCoreUtils();
  ~vtkCoreUtils();
};
#endif
