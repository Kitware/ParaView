// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAnglePropertyWidget_h
#define pqAnglePropertyWidget_h

#include "pqInteractivePropertyWidget.h"

#include <QScopedPointer>

#include <array>

/**
 * pqAnglePropertyWidget is a custom property widget that uses
 * "PolyLineWidgetRepresentation" to help users interactively set points that
 * form an angle defined by 3 point. To use this widget for a property group (vtkSMPropertyGroup),
 * use "InteractiveAngle" as the "panel_widget" in the XML configuration.
 * The property group can have properties for following functions:
 * \li \c HandlePositions: a repeatable 3-tuple vtkSMDoubleVectorProperty that
 * corresponds to the property used to set the selected spline points.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqAnglePropertyWidget : public pqInteractivePropertyWidget
{
  Q_OBJECT;
  Q_PROPERTY(QList<QVariant> points READ points WRITE setPoints NOTIFY pointsChanged);
  typedef pqInteractivePropertyWidget Superclass;

public:
  pqAnglePropertyWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqAnglePropertyWidget() override;

  ///@{
  /**
   * Get/Set the points that form the angle. Size should always be 9.
   */
  QList<QVariant> points() const;
  void setPoints(const QList<QVariant>& points);
  ///@}

Q_SIGNALS:
  /**
   * Signal fired whenever the points are changed.
   */
  void pointsChanged();

protected Q_SLOTS:
  void placeWidget() override;
  void updateLabels();

private:
  Q_DISABLE_COPY(pqAnglePropertyWidget)

  struct pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
