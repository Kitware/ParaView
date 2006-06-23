/*=========================================================================

   Program: ParaView
   Module:    pqImplicitPlaneWidget.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

=========================================================================*/

#ifndef _pqImplicitPlaneWidget_h
#define _pqImplicitPlaneWidget_h

#include "pqSMProxy.h"

#include <QWidget>

/// Provides a complete Qt UI for working with a 3D plane widget
class pqImplicitPlaneWidget :
  public QWidget
{
  Q_OBJECT
  
public:
  pqImplicitPlaneWidget(QWidget* p);
  ~pqImplicitPlaneWidget();

  /// Sets a source proxy that will be used to specify the bounding-box for the 3D widget
  void setBoundingBoxProxy(pqSMProxy proxy);
  /// Returns the current state of the widget
  void getWidgetState(double origin[3], double normal[3]);
  /// Sets the current state of the widget
  void setWidgetState(const double origin[3], const double normal[3]);

public slots:
  /// Makes the 3D widget visible (but respects the user's choice if they've turned visibility off)
  void showWidget();
  /// Makes the 3D widget plane visible (respects the overall visibility flag)
  void showPlane();
  /// Hides the 3D widget plane
  void hidePlane();
  /// Hides the 3D widget
  void hideWidget();

signals:
  /// Notifies observers that the user is dragging the 3D widget
  void widgetStartInteraction();
  /// Notifies observers that the widget has been modified
  void widgetChanged();
  /// Notifies observers that the user is done dragging the 3D widget
  void widgetEndInteraction();

private slots:
  /// Called to show/hide the 3D widget
  void onShow3DWidget(bool);
  /// Called to reset the 3D widget bounds to the bounding box proxy's
  void onResetBounds();
  /// Called to set the widget origin to the center of the bounding box proxy's data
  void onUseCenterBounds();
  /// Called to set the widget normal to the X axis
  void onUseXNormal();
  /// Called to set the widget normal to the Y axis
  void onUseYNormal();
  /// Called to set the widget normal to the Z axis
  void onUseZNormal();
  /// Called to set the widget normal to the camera direction
  void onUseCameraNormal();
  /// Called if any of the Qt widget values is modified
  void onQtWidgetChanged();
  /// Called when the user starts dragging the 3D widget
  void on3DWidgetStartDrag();
  /// Called when the 3D widget is modified
  void on3DWidgetChanged();
  /// Called when the user stops dragging the 3D widget
  void on3DWidgetEndDrag();

private:
  void show3DWidget(bool show);
  void updateQtWidgets(const double* origin, const double* normal);
  void update3DWidget(const double* origin, const double* normal);

  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif
