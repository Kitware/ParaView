// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqFourLinesPropertyWidget_h
#define pqFourLinesPropertyWidget_h

#include "pqInteractivePropertyWidget.h"
#include <QScopedPointer>
class QColor;
class QLabel;

/**
 * pqFourLinesPropertyWidget is a custom property widget that uses
 * "FourLinesSourceWidgetRepresentation" to help users interactively select the end
 * points of a line. To use this widget for a property group
 * (vtkSMPropertyGroup), use "InteractiveFourLines" as the "panel_widget" in the
 * XML configuration for the proxy. The property group should have properties for
 * following functions:
 * \li \c Point1WorldPositions: a 3-tuple vtkSMDoubleVectorProperty that will be
 * linked to the end points of the lines.
 * \li \c Point2WorldPosition: a 3-tuple vtkSMDoubleVectorProperty that will be
 * linked to the others end points of the lines.
 * \li \c Input: (optional) a vtkSMInputProperty that is used to get data
 * information for bounds when placing/resetting the widget.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqFourLinesPropertyWidget : public pqInteractivePropertyWidget
{
  Q_OBJECT
  typedef pqInteractivePropertyWidget Superclass;

public:
  pqFourLinesPropertyWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqFourLinesPropertyWidget() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Switch the 2 end points of a line
   */
  void switchPoints(int index);

  /**
   * Set the color to use for the line widget.
   */
  void setLineColor(const QColor& color);

protected Q_SLOTS:
  /**
   * Places the interactive widget using current data source information.
   */
  void placeWidget() override;

  ///@{
  /**
   * Updates the length labels.
   */
  void updateLengthLabels();
  void updateLengthLabel(QLabel* lengthLabel, int index);
  ///@}

private:
  Q_DISABLE_COPY(pqFourLinesPropertyWidget)
  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
