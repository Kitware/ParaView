// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqHandlePropertyWidget_h
#define pqHandlePropertyWidget_h

#include "pqInteractivePropertyWidget.h"

class QPushButton;

/**
 * pqHandlePropertyWidget is a custom property widget that uses
 * "HandleWidgetRepresentation" to help users interactively set a 3D point in
 * space. To use this widget for a property group
 * (vtkSMPropertyGroup), use "InteractiveHandle" as the "panel_widget" in the
 * XML configuration for the proxy. The property group should have properties for
 * following functions:
 * \li \c WorldPosition: a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
 * origin of the interactive plane.
 * \li \c Input: (optional) a vtkSMInputProperty that is used to get data
 * information for bounds when placing/resetting the widget.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqHandlePropertyWidget : public pqInteractivePropertyWidget
{
  Q_OBJECT
  typedef pqInteractivePropertyWidget Superclass;

public:
  pqHandlePropertyWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqHandlePropertyWidget() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Update the widget's WorldPosition using current data bounds.
   */
  void centerOnBounds();

protected Q_SLOTS:
  /**
   * Places the interactive widget using current data source information.
   */
  void placeWidget() override;

private Q_SLOTS:
  void setWorldPosition(double x, double y, double z);
  void selectionChanged();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqHandlePropertyWidget);
  QPushButton* UseSelectionCenterButton = nullptr;
};

#endif
