// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqOSPRayHidingDecorator_h
#define pqOSPRayHidingDecorator_h

#include "pqPropertyWidgetDecorator.h"

#include "vtkOSPRayHidingDecorator.h"

/**
 * pqOSPRayHidingDecorator's purpose is to prevent the GUI from
 * showing any of the RayTracing specific rendering controls when
 * Paraview is not configured with PARAVIEW_ENABLE_RAYTRACING
 *
 * @see vtkOSPRayHidingDecorator
 */
class pqOSPRayHidingDecorator : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  pqOSPRayHidingDecorator(vtkPVXMLElement* config, pqPropertyWidget* parentObject);
  ~pqOSPRayHidingDecorator() override;

  /**
   * Overridden to hide the widget when OSPRay is not compiled in
   */
  bool canShowWidget(bool show_advanced) const override;

private:
  Q_DISABLE_COPY(pqOSPRayHidingDecorator)
  vtkNew<vtkOSPRayHidingDecorator> decoratorLogic;
};

#endif
