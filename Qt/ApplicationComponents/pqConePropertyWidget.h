// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqConePropertyWidget_h
#define pqConePropertyWidget_h

#include "pqInteractivePropertyWidget.h"

#include <array>

class QWidget;
class vtkVector3d;

/**
 * pqConePropertyWidget is a custom property widget that uses
 * "ImplicitConeWidgetRepresentation" to help users interactively set the origin, angle
 * and axis for a cone. To use this widget for a property group
 * (vtkSMPropertyGroup), use "InteractiveCone" as the "panel_widget" in the
 * XML configuration for the proxy. The property group should have properties for
 * following functions:
 * \li \c Origin : a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
 * origin of the cone
 * \li \c Axis : a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
 * axis for the cone
 * \li \c Angle: a 1-tuple vtkSMDoubleVectorProperty that will be linked to the
 * angle for the cone
 * \li \c Input: (optional) a vtkSMInputProperty that is used to get data
 * information for bounds when placing/resetting the widget.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqConePropertyWidget : public pqInteractivePropertyWidget
{
  Q_OBJECT
  typedef pqInteractivePropertyWidget Superclass;

public:
  pqConePropertyWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqConePropertyWidget() override = default;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set the cone axis to be along the X axis.
   */
  void useXAxis();

  /**
   * Set the cone axis to be along the Y axis.
   */
  void useYAxis();

  /**
   * Set the cone axis to be along the Z axis.
   */
  void useZAxis();

  /**
   * Reset the camera to be down the cone axis.
   */
  void resetCameraToAxis();

  /**
   * Set the cone axis to be along the camera view direction.
   */
  void useCameraAxis();

private Q_SLOTS:
  /**
   * Places the interactive widget using current data source information.
   */
  void placeWidget() override;

  void resetBounds();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqConePropertyWidget)

  void setAxis(const vtkVector3d& axis);
  void updateWidget(bool showingAdvancedProperties) override;

  pqPropertyLinks WidgetLinks;
  std::array<QWidget*, 2> AdvancedPropertyWidgets;
};

#endif
