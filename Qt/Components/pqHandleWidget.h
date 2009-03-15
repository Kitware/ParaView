/*=========================================================================

   Program:   ParaQ
   Module:    pqHandleWidget.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.2. 

   See License_v1.2.txt for the full ParaQ license.
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
#include "pqComponentsExport.h"

/// Provides a complete Qt UI for working with a 3D handle widget
class PQCOMPONENTS_EXPORT pqHandleWidget : public pq3DWidget
{
  Q_OBJECT
  
public:
  typedef pq3DWidget Superclass;

  pqHandleWidget(vtkSMProxy* refProxy, vtkSMProxy* pxy, QWidget* p);
  ~pqHandleWidget();

  /// Resets the bounds of the 3D widget to the reference proxy bounds.
  /// This typically calls PlaceWidget on the underlying 3D Widget 
  /// with reference proxy bounds.
  /// This should be explicitly called after the panel is created
  /// and the widget is initialized i.e. the reference proxy, controlled proxy
  /// and hints have been set.
  virtual void resetBounds()
    { this->Superclass::resetBounds(); }
  virtual void resetBounds(double bounds[6]);

private slots:
  /// Called when the user changes widget visibility
  void onWidgetVisibilityChanged(bool visible);

protected:
  /// Internal method to create the widget.
  void createWidget(pqServer*);
  
  /// Internal method to cleanup widget.
  void cleanupWidget();

  /// Called on pick.
  virtual void pick(double, double, double);

private:
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif
