// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqOpenVRHidingDecorator_h
#define pqOpenVRHidingDecorator_h

#include "pqPropertyWidgetDecorator.h"

/// pqOpenVRHidingDecorator's purpose is to prevent the GUI from
/// showing any of the OpenVR specific rendering controls when
/// Paraview is not configured with PARAVIEW_USE_OpenVR
class pqOpenVRHidingDecorator : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  pqOpenVRHidingDecorator(vtkPVXMLElement* config, pqPropertyWidget* parentObject);
  ~pqOpenVRHidingDecorator() override;

  /// Overridden to hide the widget when OpenVR is not compiled in
  bool canShowWidget(bool show_advanced) const override;

private:
  Q_DISABLE_COPY(pqOpenVRHidingDecorator)
};

#endif
