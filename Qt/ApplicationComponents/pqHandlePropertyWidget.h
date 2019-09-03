/*=========================================================================

   Program: ParaView
   Module:  pqHandlePropertyWidget.h

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
  pqHandlePropertyWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = 0);
  ~pqHandlePropertyWidget() override;

public slots:
  /**
  * Update the widget's WorldPosition using current data bounds.
  */
  void centerOnBounds();

protected slots:
  /**
  * Places the interactive widget using current data source information.
  */
  void placeWidget() override;

private slots:
  void setWorldPosition(double x, double y, double z);
  void selectionChanged();

private:
  Q_DISABLE_COPY(pqHandlePropertyWidget);
  QPushButton* UseSelectionCenterButton = nullptr;
};

#endif
