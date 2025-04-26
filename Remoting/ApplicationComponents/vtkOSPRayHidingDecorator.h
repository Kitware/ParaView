// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkOSPRayHidingDecorator_h
#define vtkOSPRayHidingDecorator_h

#include "vtkPropertyDecorator.h"

/**
 * vtkOSPRayHidingDecorator's purpose is to prevent the GUI from
 * showing any of the RayTracing specific rendering controls when
 * Paraview is not configured with PARAVIEW_ENABLE_RAYTRACING
 */
class VTKREMOTINGAPPLICATIONCOMPONENTS_EXPORT vtkOSPRayHidingDecorator : public vtkPropertyDecorator
{

public:
  static vtkOSPRayHidingDecorator* New();
  vtkTypeMacro(vtkOSPRayHidingDecorator, vtkPropertyDecorator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to hide the widget when OSPRay is not compiled in
   */
  bool CanShow(bool show_advanced) const override;

protected:
  vtkOSPRayHidingDecorator();
  ~vtkOSPRayHidingDecorator() override;

private:
  vtkOSPRayHidingDecorator(const vtkOSPRayHidingDecorator&) = delete;
  void operator=(const vtkOSPRayHidingDecorator&) = delete;
};
#endif
