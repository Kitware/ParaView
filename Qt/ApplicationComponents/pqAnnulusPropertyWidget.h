// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqAnnulusPropertyWidget_h
#define pqAnnulusPropertyWidget_h

#include "pqInteractivePropertyWidget.h"

#include <array>

class QWidget;
class vtkVector3d;

/**
 * pqAnnulusPropertyWidget is a custom property widget that uses
 * "ImplicitAnnulusWidgetRepresentation" to help users interactively set the center, inner/outer
 * radii and axis for a annulus. To use this widget for a property group (vtkSMPropertyGroup), use
 * "InteractiveAnnulus" as the "panel_widget" in the XML configuration for the proxy. The property
 * group should have properties for following functions: \li \c Center : a 3-tuple
 * vtkSMDoubleVectorProperty that will be linked to the center of the annulus \li \c Axis : a
 * 3-tuple vtkSMDoubleVectorProperty that will be linked to the axis for the annulus \li \c
 * InnerRadius: a 1-tuple vtkSMDoubleVectorProperty that will be linked to the inner radius for the
 * annulus \li \c OuterRadius: a 1-tuple vtkSMDoubleVectorProperty that will be linked to the outer
 * radius for the annulus \li \c Input: (optional) a vtkSMInputProperty that is used to get data
 * information for bounds when placing/resetting the widget.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqAnnulusPropertyWidget : public pqInteractivePropertyWidget
{
  Q_OBJECT
  typedef pqInteractivePropertyWidget Superclass;

public:
  pqAnnulusPropertyWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqAnnulusPropertyWidget() override = default;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set the annulus axis to be along the X axis.
   */
  void useXAxis();

  /**
   * Set the annulus axis to be along the Y axis.
   */
  void useYAxis();

  /**
   * Set the annulus axis to be along the Z axis.
   */
  void useZAxis();

  /**
   * Reset the camera to be down the annulus axis.
   */
  void resetCameraToAxis();

  /**
   * Set the annulus axis to be along the camera view direction.
   */
  void useCameraAxis();

private Q_SLOTS:
  /**
   * Places the interactive widget using current data source information.
   */
  void placeWidget() override;

  void resetBounds();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqAnnulusPropertyWidget)

  void setAxis(const vtkVector3d& axis);
  void updateWidget(bool showingAdvancedProperties) override;

  pqPropertyLinks WidgetLinks;
  std::array<QWidget*, 2> AdvancedPropertyWidgets;
};

#endif
