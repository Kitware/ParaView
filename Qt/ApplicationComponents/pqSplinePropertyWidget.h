// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSplinePropertyWidget_h
#define pqSplinePropertyWidget_h

#include "pqInteractivePropertyWidget.h"

#include <QScopedPointer>

class QColor;

/**
 * pqSplinePropertyWidget is a custom property widget that uses
 * "SplineWidgetRepresentation" to help users interactively set points that
 * form a spline. To use this widget for a property group (vtkSMPropertyGroup),
 * use "InteractiveSpline" as the "panel_widget" in the XML configuration.
 * The property group can have properties for following functions:
 * \li \c HandlePositions: a repeatable 3-tuple vtkSMDoubleVectorProperty that
 * corresponds to the property used to set the selected spline points.
 * \li \c Closed: (optional) a 1-tuple vtkSMIntVectorProperty that
 * corresponds to the boolean flag indicating if the spline should be closed at end points.
 * \li \c Input: (optional) a vtkSMInputProperty that is used to get data
 * information for bounds when placing/resetting the widget.
 * This widget can also be used for a poly-line instead of a spline. For this mode, use
 * "InteractivePolyLine" as the "panel_widget" in the XML configuration.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSplinePropertyWidget : public pqInteractivePropertyWidget
{
  Q_OBJECT;
  Q_PROPERTY(QList<QVariant> points READ points WRITE setPoints NOTIFY pointsChanged);
  Q_PROPERTY(int currentRow READ currentRow WRITE setCurrentRow NOTIFY currentRowChanged);
  typedef pqInteractivePropertyWidget Superclass;

public:
  enum ModeTypes
  {
    SPLINE = 0,
    POLYLINE = 1
  };

  pqSplinePropertyWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, ModeTypes mode = SPLINE,
    QWidget* parent = nullptr);
  ~pqSplinePropertyWidget() override;

  ///@{
  /**
   * Get/Set the points that form the spline.
   */
  QList<QVariant> points() const;
  void setPoints(const QList<QVariant>& points);
  ///@}

  ///@{
  int currentRow() const;
  void setCurrentRow(int idx);
  ///@}
Q_SIGNALS:
  /**
   * Signal fired whenever the points are changed.
   */
  void pointsChanged();

  /**
   * Signal fired when the current row selected in the widget is changed.
   */
  void currentRowChanged();

public Q_SLOTS:
  /**
   * Set the color to use for the spline.
   */
  void setLineColor(const QColor&);

protected Q_SLOTS:
  void placeWidget() override;

private:
  Q_DISABLE_COPY(pqSplinePropertyWidget)
  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
