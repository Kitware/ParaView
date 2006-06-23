/*=========================================================================

   Program: ParaView
   Module:    pqWidgetObjectPanel.h

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

#ifndef _pqWidgetObjectPanel_h
#define _pqWidgetObjectPanel_h

#include "pqLoadedFormObjectPanel.h"

class vtkSMNew3DWidgetProxy;

class pqWidgetObjectPanel :
  public pqLoadedFormObjectPanel
{
  Q_OBJECT
public:
  /// constructor
  pqWidgetObjectPanel(QString filename, QWidget* p);
  /// destructor
  ~pqWidgetObjectPanel();

public slots:
  /// Called when the panel becomes active. 
  /// Overridden to enable the 3D Widget. 
  virtual void select();

  /// Called when the panel becomes inactive. 
  /// Overridden to disable the 3D widget.
  virtual void deselect();
  
protected:
  /// set the proxy to display properties for
  virtual void setProxyInternal(pqSMProxy proxy);

  vtkSMNew3DWidgetProxy* Widget;

private slots:
  /// Called to reset the 3D widget bounds to the source data
  void onResetBounds();
  /// Called to set the widget origin to the center of the source data
  void onUseCenterBounds();
  /// Called to set the widget normal to the X axis
  void onUseXNormal();
  /// Called to set the widget normal to the Y axis
  void onUseYNormal();
  /// Called to set the widget normal to the Z axis
  void onUseZNormal();
  /// Called to set the widget normal to the camera direction
  void onUseCameraNormal();

private:
  class WidgetObserver;
  friend class WidgetObserver;
  WidgetObserver* const Observer;
  
  /// Derivatives should override this to receive a notification that the 3D widget has changed
  virtual void on3DWidgetChanged();
};

#endif

