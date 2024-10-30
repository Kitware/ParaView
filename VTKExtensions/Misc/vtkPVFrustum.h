// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVFrustum
 * @brief   extends vtkFrustum to add ParaView specific API.
 *
 * vtkPVFrustum extends vtkFrustum to add ParaView specific API. In this case, Orientation and
 * Origin members and associated setters.
 */

#ifndef vtkPVFrustum_h
#define vtkPVFrustum_h

#include "vtkFrustum.h"
#include "vtkPVVTKExtensionsMiscModule.h" //needed for exports
#include "vtkVector.h"                    // For vtkVector3d

class VTKPVVTKEXTENSIONSMISC_EXPORT vtkPVFrustum : public vtkFrustum
{
public:
  static vtkPVFrustum* New();

  vtkTypeMacro(vtkPVFrustum, vtkFrustum);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set orientation of the frustum. Defaults to (0,0,0)
   */
  void SetOrientation(double x, double y, double z);
  void SetOrientation(const double xyz[3]);
  void SetOrientation(const vtkVector3d& xyz);
  double* GetOrientation();
  void GetOrientation(double& x, double& y, double& z);
  void GetOrientation(double xyz[3]);
  ///@}

  ///@{
  /**
   * Get/Set orientation of the frustum. Defaults to (0,0,0)
   */
  void SetOrigin(double x, double y, double z);
  void SetOrigin(const double xyz[3]);
  void SetOrigin(const vtkVector3d& xyz);
  double* GetOrigin();
  void GetOrigin(double& x, double& y, double& z);
  void GetOrigin(double xyz[3]);
  /// @}

private:
  vtkPVFrustum() = default;
  ~vtkPVFrustum() override = default;

  vtkPVFrustum(const vtkPVFrustum&) = delete;
  void operator=(const vtkPVFrustum&) = delete;

  // Compute and set transform according to orienation and origin. Used in setters.
  void UpdateTransform();

  vtkVector3d Orientation = { 0, 0, 0 };
  vtkVector3d Origin = { 0, 0, 0 };
};

#endif
