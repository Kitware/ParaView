/*=========================================================================

   Program: ParaView
   Module:  pqLinePropertyWidget.h

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
  pqLinePropertyWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = 0);
  ~pqLinePropertyWidget() override;

public Q_SLOTS:
  void useXAxis() { this->useAxis(0); }
  void useYAxis() { this->useAxis(1); }
  void useZAxis() { this->useAxis(2); }
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
