// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLinePropertyWidget_h
#define pqLinePropertyWidget_h

#include "pqInteractivePropertyWidget.h"
#include <QScopedPointer>
class QColor;

/**
 * pqLinePropertyWidget is a custom property widget that uses
 * "LineWidgetRepresentation" to help the users
 */

/**
 * pqLinePropertyWidget is a custom property widget that uses
 * "LineSourceWidgetRepresentation" to help users interactively select the end
 * points of a line. To use this widget for a property group
 * (vtkSMPropertyGroup), use "InteractiveLine" as the "panel_widget" in the
 * XML configuration for the proxy. The property group should have properties for
 * following functions:
 * \li \c Point1WorldPosition: a 3-tuple vtkSMDoubleVectorProperty that will be
 * linked to one of the end points of the line.
 * \li \c Point2WorldPosition: a 3-tuple vtkSMDoubleVectorProperty that will be
 * linked to the other end point of the line.
 * \li \c Input: (optional) a vtkSMInputProperty that is used to get data
 * information for bounds when placing/resetting the widget.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqLinePropertyWidget : public pqInteractivePropertyWidget
{
  Q_OBJECT
  typedef pqInteractivePropertyWidget Superclass;

public:
  pqLinePropertyWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqLinePropertyWidget() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void useXAxis() { this->useAxis(0); }
  void useYAxis() { this->useAxis(1); }
  void useZAxis() { this->useAxis(2); }
  void flipP2();
  void centerOnBounds();

  /**
   * Set the color to use for the line widget.
   */
  void setLineColor(const QColor& color);

protected Q_SLOTS:
  /**
   * Places the interactive widget using current data source information.
   */
  void placeWidget() override;

  /**
   * Called when user picks a point using the pick shortcut keys.
   */
  void pick(double x, double y, double z);
  void pickPoint1(double x, double y, double z);
  void pickPoint2(double x, double y, double z);
  void pickNormal(double x, double y, double z, double nx, double ny, double nz);

  /**
   * Updates the length label.
   */
  void updateLengthLabel();

private:
  Q_DISABLE_COPY(pqLinePropertyWidget)
  class pqInternals;
  QScopedPointer<pqInternals> Internals;
  vtkBoundingBox referenceBounds() const;

  void useAxis(int axis);
};

#endif
