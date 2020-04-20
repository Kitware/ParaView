/*=========================================================================

   Program: ParaView
   Module:  pqCylinderPropertyWidget.h

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
#ifndef pqCylinderPropertyWidget_h
#define pqCylinderPropertyWidget_h

#include "pqInteractivePropertyWidget.h"

class QWidget;

/**
* pqCylinderPropertyWidget is a custom property widget that uses
* "ImplicitCylinderWidgetRepresentation" to help users interactively set the center, radius
* and axis for a cylinder. To use this widget for a property group
* (vtkSMPropertyGroup), use "InteractiveCylinder" as the "panel_widget" in the
* XML configuration for the proxy. The property group should have properties for
* following functions:
* \li \c Center : a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
* center of the cylinder
* \li \c Axis : a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
* axis for the cylinder
* \li \c Radius: a 1-tuple vtkSMDoubleVectorProperty that will be linked to the
* radius for the cylinder
* \li \c Input: (optional) a vtkSMInputProperty that is used to get data
* information for bounds when placing/resetting the widget.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqCylinderPropertyWidget : public pqInteractivePropertyWidget
{
  Q_OBJECT
  typedef pqInteractivePropertyWidget Superclass;

public:
  pqCylinderPropertyWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = 0);
  ~pqCylinderPropertyWidget() override;

public Q_SLOTS:
  /**
  * Set the cylinder axis to be along the X axis.
  */
  void useXAxis() { this->setAxis(1, 0, 0); }

  /**
  * Set the cylinder axis to be along the Y axis.
  */
  void useYAxis() { this->setAxis(0, 1, 0); }

  /**
  * Set the cylinder axis to be along the Z axis.
  */
  void useZAxis() { this->setAxis(0, 0, 1); }

  /**
  * Reset the camera to be down the cylinder axis.
  */
  void resetCameraToAxis();

  /**
  * Set the cylinder axis to be along the camera view direction.
  */
  void useCameraAxis();

protected Q_SLOTS:
  /**
  * Places the interactive widget using current data source information.
  */
  void placeWidget() override;

  void resetBounds();

private:
  Q_DISABLE_COPY(pqCylinderPropertyWidget)

  void setAxis(double x, double y, double z);
  void updateWidget(bool showing_advanced_properties) override;

  pqPropertyLinks WidgetLinks;
  QWidget* AdvancedPropertyWidgets[2];
};

#endif
