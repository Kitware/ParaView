// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVPlane
 * @brief   extends vtkPlane to add Offset parameter.
 *
 * vtkPVPlane adds an offset setting to vtkPlane.
 * This offset is used together with normal and origin when
 * setting parameters on the represented object.
 */

#ifndef vtkPVPlane_h
#define vtkPVPlane_h

#include "vtkPVVTKExtensionsMiscModule.h" // Needed for exports
#include "vtkParaViewDeprecation.h"       // For PARAVIEW_DEPRECATED_IN_6_0_0
#include "vtkPlane.h"

class PARAVIEW_DEPRECATED_IN_6_0_0(
  "Use vtkPlane instead (vtkPlane now supports AxisAligned and Offset).")
  VTKPVVTKEXTENSIONSMISC_EXPORT vtkPVPlane : public vtkPlane
{
public:
  static vtkPVPlane* New();
  vtkTypeMacro(vtkPVPlane, vtkPlane);

protected:
  vtkPVPlane();
  ~vtkPVPlane() override;

private:
  vtkPVPlane(const vtkPVPlane&) = delete;
  void operator=(const vtkPVPlane&) = delete;
};

#endif
