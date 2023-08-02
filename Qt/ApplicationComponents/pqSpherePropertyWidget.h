// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSpherePropertyWidget_h
#define pqSpherePropertyWidget_h

#include "pqInteractivePropertyWidget.h"

/**
 * pqSpherePropertyWidget is a custom property widget that uses
 * "SphereWidgetRepresentation" to help users interactively setup a center and
 * a radius for a group of properties used to define a spherical shape. To use
 * this widget for a property group (vtkSMPropertyGroup), use
 * "InteractiveSphere" as the "panel_widget" in the XML configuration.
 * The property group should have properties for the following functions:
 * \li \c Center: a 3-tuple vtkSMDoubleVectorProperty that corresponds to the center of the Sphere.
 * \li \c Radius: a 1-tuple vtkSMDoubleVectorProperty that corresponds to the radius.
 * \li \c Normal: (optional) a 3-tuple vtkSMDoubleVectorProperty corresponds to a direction vector.
 * \li \c Input: (optional) a vtkSMInputProperty that is used to get data
 * information for bounds when placing/resetting the widget.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSpherePropertyWidget : public pqInteractivePropertyWidget
{
  Q_OBJECT
  typedef pqInteractivePropertyWidget Superclass;

public:
  pqSpherePropertyWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqSpherePropertyWidget() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Center the widget on the data bounds.
   */
  void centerOnBounds();

protected Q_SLOTS:
  /**
   * Places the interactive widget using current data source information.
   */
  void placeWidget() override;

private Q_SLOTS:
  void setCenter(double x, double y, double z);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqSpherePropertyWidget)
};

#endif
