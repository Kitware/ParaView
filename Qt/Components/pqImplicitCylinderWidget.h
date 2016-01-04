/*=========================================================================

   Program: ParaView
   Module:    pqImplicitCylinderWidget.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#ifndef _pqImplicitCylinderWidget_h
#define _pqImplicitCylinderWidget_h

#include "pq3DWidget.h"

class pqServer;

/// Provides a complete Qt UI for working with a 3D cylinder widget
class PQCOMPONENTS_EXPORT pqImplicitCylinderWidget : public pq3DWidget
{
  Q_OBJECT
  typedef pq3DWidget Superclass;

public:
  pqImplicitCylinderWidget(vtkSMProxy* refProxy, vtkSMProxy* proxy, QWidget* p = 0);
  ~pqImplicitCylinderWidget();

public slots:
  /// Resets the bounds of the 3D widget to the reference proxy bounds.
  virtual void resetBounds()
    { this->Superclass::resetBounds(); }
  virtual void resetBounds(double bounds[6]);

  /// accept the changes
  void accept();

  /// reset the changes
  void reset();

  /// Overridden to update widget placement based on data bounds.
  virtual void select();

protected:
  /// Makes the 3D widget cylinder visible (respects the overall visibility flag)
  virtual void showCylinder();

  /// Hides the 3D widget cylinder
  virtual void hideCylinder();

  /// Internal method to create the widget.
  void createWidget(pqServer*);

private slots:
  /// Called to show/hide the 3D widget
  void onShow3DWidget(bool);
  void onWidgetVisibilityChanged(bool visible);

  void onTubing(bool);
  void onOutlineTranslation(bool);
  void onOutsideBounds(bool);
  void onScaling(bool);

private:
  void setNormalProperty(vtkSMProperty*);
  void setOriginProperty(vtkSMProperty*);

  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif
