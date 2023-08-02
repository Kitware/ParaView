// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLightPropertyWidget_h
#define pqLightPropertyWidget_h

#include "pqInteractivePropertyWidget.h"

/**
 * pqLightPropertyWidget is a custom property widget that uses
 * "LightWidgetRepresentation" to help users interactively set a light properties in
 * space. To use this widget for a property group
 * (vtkSMPropertyGroup), use "InteractiveLight" as the "panel_widget" in the
 * XML configuration for the proxy. The property group should have properties for
 * following functions:
 * \li \c Positional: a 1-tuple vtkSMIntVectorProperty with a vtkSMEnumerationDomain
 * that will be use to show the positional cone or not.
 * \li \c WorldPosition: a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
 * origin of the interactive light.
 * \li \c FocalPoint: a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
 * the focal point of the interactive light.
 * \li \c FocalPoint: a 1-tuple vtkSMDoubleVectorProperty with a vtkSMDoubleRangeDomain
 * that will be linked to the cone angle of the interactive light.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqLightPropertyWidget : public pqInteractivePropertyWidget
{
  Q_OBJECT
  typedef pqInteractivePropertyWidget Superclass;

public:
  pqLightPropertyWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqLightPropertyWidget() override;

protected Q_SLOTS:
  void placeWidget() override {}

  /**
   * Update the visibility state of the different elements of the property widget
   */
  void updateVisibilityState();

private:
  Q_DISABLE_COPY(pqLightPropertyWidget);

  class pqInternals;
  pqInternals* Internals;
};

#endif
