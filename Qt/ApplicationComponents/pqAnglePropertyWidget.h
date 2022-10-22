/*=========================================================================

   Program: ParaView
   Module:  pqAnglePropertyWidget.h

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
