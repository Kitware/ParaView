// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCylinderPropertyWidget_h
#define pqCylinderPropertyWidget_h

#include "pqInteractivePropertyWidget.h"

class QWidget;

/**
 * pqCylinderPropertyWidget is a custom property widget that uses
 * "ImplicitCylinderWidgetRepresentation" to help users interactively set the center, radius
 * and axis for a cylinder. To use this widget for a property group
 * (vtkSMPropertyGroup), use "InteractiveCylinder" as the "panel_widget" in the
 * XML configuration for the proxy. The property group should have properties for
 * following functions:
 * \li \c Center : a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
 * center of the cylinder
 * \li \c Axis : a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
 * axis for the cylinder
 * \li \c Radius: a 1-tuple vtkSMDoubleVectorProperty that will be linked to the
 * radius for the cylinder
 * \li \c Input: (optional) a vtkSMInputProperty that is used to get data
 * information for bounds when placing/resetting the widget.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCylinderPropertyWidget : public pqInteractivePropertyWidget
{
  Q_OBJECT
  typedef pqInteractivePropertyWidget Superclass;

public:
  pqCylinderPropertyWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqCylinderPropertyWidget() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set the cylinder axis to be along the X axis.
   */
  void useXAxis() { this->setAxis(1, 0, 0); }

  /**
   * Set the cylinder axis to be along the Y axis.
   */
  void useYAxis() { this->setAxis(0, 1, 0); }

  /**
   * Set the cylinder axis to be along the Z axis.
   */
  void useZAxis() { this->setAxis(0, 0, 1); }

  /**
   * Reset the camera to be down the cylinder axis.
   */
  void resetCameraToAxis();

  /**
   * Set the cylinder axis to be along the camera view direction.
   */
  void useCameraAxis();

protected Q_SLOTS:
  /**
   * Places the interactive widget using current data source information.
   */
  void placeWidget() override;

  void resetBounds();

private:
  Q_DISABLE_COPY(pqCylinderPropertyWidget)

  void setAxis(double x, double y, double z);
  void updateWidget(bool showing_advanced_properties) override;

  pqPropertyLinks WidgetLinks;
  QWidget* AdvancedPropertyWidgets[2];
};

#endif
