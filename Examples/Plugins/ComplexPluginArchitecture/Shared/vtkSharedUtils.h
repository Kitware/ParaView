// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSharedUtils
 * @brief   provide shared utils
 */

#ifndef vtkSharedUtils_h
#define vtkSharedUtils_h

#include "SharedModule.h" // for export macro

#include "vtkObject.h"

class SHARED_EXPORT vtkSharedUtils : public vtkObject
{
public:
  static vtkSharedUtils* New();
  vtkTypeMacro(vtkSharedUtils, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * A mathematical constant.
   */
  static double Pi();

  ///@{
  /**
   * Convert radians into degrees
   */
  static float DegreesFromRadians(float radians);
  static double DegreesFromRadians(double radians);
  ///@}

  vtkSharedUtils(const vtkSharedUtils&) = delete;
  void operator=(const vtkSharedUtils&) = delete;

protected:
  vtkSharedUtils();
  ~vtkSharedUtils();
};
#endif
