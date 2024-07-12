// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPVCone
 * @brief   extends vtkCone to add ParaView specific API.
 *
 * vtkPVCone extends vtkCone to add ParaView specific API. Not that unlike vtkCone, vtkPVCone is
 * oriented and represent a one-sided cone.
 */

#ifndef vtkPVCone_h
#define vtkPVCone_h

#include "vtkCone.h"
#include "vtkPVVTKExtensionsMiscModule.h" //needed for exports

class VTKPVVTKEXTENSIONSMISC_EXPORT vtkPVCone : public vtkCone
{
public:
  static vtkPVCone* New();

  vtkTypeMacro(vtkPVCone, vtkCone);

  ///@{
  /**
   * Get/Set the vector defining the direction of the cone.
   */
  void SetAxis(double x, double y, double z) override;
  void SetAxis(double axis[3]) override;
  ///@}

  // Reimplemented to update transform on change:
  void SetOrigin(double x, double y, double z) override;
  void SetOrigin(const double xyz[3]) override;

  /**
   * Evaluate cone equation.
   */
  double EvaluateFunction(double x[3]) override;

  /**
   * Evaluate cone normal.
   */
  void EvaluateGradient(double x[3], double g[3]) override;

private:
  vtkPVCone() = default;
  ~vtkPVCone() override = default;

  vtkPVCone(const vtkPVCone&) = delete;
  void operator=(const vtkPVCone&) = delete;

  void UpdateTransform();
};

#endif
