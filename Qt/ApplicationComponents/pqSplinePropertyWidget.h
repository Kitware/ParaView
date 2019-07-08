/*=========================================================================

   Program: ParaView
   Module:  pqSplinePropertyWidget.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
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

  pqSplinePropertyWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, ModeTypes mode = SPLINE, QWidget* parent = 0);
  ~pqSplinePropertyWidget() override;

  //@{
  /**
   * Get/Set the points that form the spline.
   */
  QList<QVariant> points() const;
  void setPoints(const QList<QVariant>& points);
  //@}

  //@{
  int currentRow() const;
  void setCurrentRow(int idx);
  //@}
signals:
  /**
   * Signal fired whenever the points are changed.
   */
  void pointsChanged();

  /**
   * Signal fired when the current row selected in the widget is changed.
   */
  void currentRowChanged();

public slots:
  /**
  * Set the color to use for the spline.
  */
  void setLineColor(const QColor&);

protected slots:
  void placeWidget() override;

private:
  Q_DISABLE_COPY(pqSplinePropertyWidget)
  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
