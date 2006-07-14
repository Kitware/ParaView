/*=========================================================================

   Program:   ParaQ
   Module:    pqHandleWidget.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#ifndef _pqHandleWidget_h
#define _pqHandleWidget_h

#include "pqProxy.h"
#include "pq3DWidget.h"

/// Provides a complete Qt UI for working with a 3D handle widget
class pqHandleWidget : public pq3DWidget
{
  Q_OBJECT
  
public:
  pqHandleWidget(QWidget* p);
  ~pqHandleWidget();

  /// Controlled proxy is a proxy which is controlled by the 3D widget.
  /// A controlled proxy must provide "Hints" describing how
  /// the properties of the controlled proxy are controlled by the
  /// 3D widget.
  virtual void setControlledProxy(vtkSMProxy*);

protected:
  /// Resets the bounds of the 3D widget to the reference proxy bounds.
  virtual void resetBounds();

private slots:
  /// Called to show/hide the 3D widget
  void onShow3DWidget(bool);
  /// Called to reset the 3D widget bounds to the reference proxy bounds
  void onResetBounds();
  /// Called when the user starts dragging the 3D widget
  void on3DWidgetStartDrag();
  /// Called when the user stops dragging the 3D widget
  void on3DWidgetEndDrag();

protected:
  virtual void setControlledProperty(const char* function,
    vtkSMProperty * controlled_property);
  
  /// Overridden to make sure that the visibility check box is
  /// updated.
  virtual void set3DWidgetVisibility(bool visible);
private:
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif
