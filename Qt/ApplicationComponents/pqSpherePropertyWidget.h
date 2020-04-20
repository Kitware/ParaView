/*=========================================================================

   Program: ParaView
   Module:  pqSpherePropertyWidget.h

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
#ifndef pqSpherePropertyWidget_h
#define pqSpherePropertyWidget_h

#include "pqInteractivePropertyWidget.h"

/**
* pqSpherePropertyWidget is a custom property widget that uses
* "SphereWidgetRepresentation" to help users interactively setup a center and
* a radius for a group of properties used to define a spherical shape. To use
* this widget for a property group (vtkSMPropertyGroup), use
* "InteractiveSphere" as the "panel_widget" in the XML configuration.
* The property group should have properties for the following functions:
* \li \c Center: a 3-tuple vtkSMDoubleVectorProperty that corresponds to the center of the Sphere.
* \li \c Radius: a 1-tuple vtkSMDoubleVectorProperty that corresponds to the radius.
* \li \c Normal: (optional) a 3-tuple vtkSMDoubleVectorProperty corresponds to a direction vector.
* \li \c Input: (optional) a vtkSMInputProperty that is used to get data
* information for bounds when placing/resetting the widget.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqSpherePropertyWidget : public pqInteractivePropertyWidget
{
  Q_OBJECT
  typedef pqInteractivePropertyWidget Superclass;

public:
  pqSpherePropertyWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = 0);
  ~pqSpherePropertyWidget() override;

public Q_SLOTS:
  /**
  * Center the widget on the data bounds.
  */
  void centerOnBounds();

protected Q_SLOTS:
  /**
  * Places the interactive widget using current data source information.
  */
  void placeWidget() override;

private Q_SLOTS:
  void setCenter(double x, double y, double z);

private:
  Q_DISABLE_COPY(pqSpherePropertyWidget)
};

#endif
