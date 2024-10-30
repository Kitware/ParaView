// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqFrustumPropertyWidget_h
#define pqFrustumPropertyWidget_h

#include "pqInteractivePropertyWidget.h"

#include <array>

class QWidget;
class vtkVector3d;

/**
 * pqFrustumPropertyWidget is a custom property widget that uses
 * "ImplicitFrustumWidgetRepresentation" to help users interactively parameterize a frustum. To use
 * this widget for a property group (vtkSMPropertyGroup), use "InteractiveFrustum" as the
 * "panel_widget" in the XML configuration for the proxy. The property group should have properties
 * for following functions:
 *  - `Origin` : a 3-tuple vtkSMDoubleVectorProperty; the center of the frustum
 *  - `Orientation` : a 3-tuple vtkSMDoubleVectorProperty; the orientation of the frustum
 *  - `HorizontalAngle`: a 1-tuple vtkSMDoubleVectorProperty; the horizontal angle of the frustum
 *  - `VerticalAngle`: a 1-tuple vtkSMDoubleVectorProperty; the vertical angle of the frustum
 *  - `NearPlaneDistance`: a 1-tuple vtkSMDoubleVectorProperty; the near plane distance of the
 * frustum
 *  - `Input`: (optional) a vtkSMInputProperty that is used to get data information for bounds when
 * placing/resetting the widget.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqFrustumPropertyWidget : public pqInteractivePropertyWidget
{
  Q_OBJECT
  typedef pqInteractivePropertyWidget Superclass;

public:
  pqFrustumPropertyWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqFrustumPropertyWidget() override = default;

private Q_SLOTS:
  /**
   * Places the interactive widget using current data source information.
   */
  void placeWidget() override;

  void resetBounds();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqFrustumPropertyWidget)

  void setOrientation(const vtkVector3d& axis);

  pqPropertyLinks WidgetLinks;
};

#endif
