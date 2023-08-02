// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqImplicitPlanePropertyWidget_h
#define pqImplicitPlanePropertyWidget_h

#include "pqInteractivePropertyWidget.h"

/**
 * pqImplicitPlanePropertyWidget is a custom property widget that uses
 * "ImplicitPlaneWidgetRepresentation" to help users interactively set the origin
 * and normal for a plane. To use this widget for a property group
 * (vtkSMPropertyGroup), use "InteractivePlane2" as the "panel_widget" in the
 * XML configuration for the proxy. The property group should have properties for
 * following functions:
 * \li \c Origin: a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
 * origin of the interactive plane.
 * \li \c Normal: a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
 * normal for the interactive plane.
 * \li \c Input: (optional) a vtkSMInputProperty that is used to get data
 * information for bounds when placing/resetting the widget.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqImplicitPlanePropertyWidget
  : public pqInteractivePropertyWidget
{
  Q_OBJECT
  typedef pqInteractivePropertyWidget Superclass;

public:
  pqImplicitPlanePropertyWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqImplicitPlanePropertyWidget() override;

  /**
   * Overridden to update the DrawPlane state.
   */
  void apply() override;
  void reset() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Slots used to toggle the visibility of the translucent plane.
   */
  void showPlane() { this->setDrawPlane(true); }
  void hidePlane() { this->setDrawPlane(false); }
  void setDrawPlane(bool val);

  /**
   * Set the widget normal to be along the X axis.
   */
  void useXNormal() { this->setNormal(1, 0, 0); }

  /**
   * Set the widget normal to be along the Y axis.
   */
  void useYNormal() { this->setNormal(0, 1, 0); }

  /**
   * Set the widget normal to be along the Z axis.
   */
  void useZNormal() { this->setNormal(0, 0, 1); }

  /**
   * Update the widget's origin and bounds using current data bounds.
   */
  void resetToDataBounds();

  /**
   * Reset the camera to be down the plane normal.
   */
  void resetCameraToNormal();

  /**
   * Set the widget normal to be along the camera view direction.
   */
  void useCameraNormal();

protected Q_SLOTS:
  /**
   * Places the interactive widget using current data source information.
   */
  void placeWidget() override;

private Q_SLOTS:
  void setOrigin(double x, double y, double z);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqImplicitPlanePropertyWidget)

  void setNormal(double x, double y, double z);
};

#endif
