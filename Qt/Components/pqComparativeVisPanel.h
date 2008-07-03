/*=========================================================================

   Program: ParaView
   Module:    pqComparativeVisPanel.h

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

========================================================================*/
#ifndef __pqComparativeVisPanel_h 
#define __pqComparativeVisPanel_h

#include <QWidget>
#include "pqComponentsExport.h"

class pqView;
class vtkSMProxy;
class vtkSMProperty;

class PQCOMPONENTS_EXPORT pqComparativeVisPanel : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;
public:
  pqComparativeVisPanel(QWidget* parent=0);
  ~pqComparativeVisPanel();

public slots:
  /// Set the view to shown in this panel. If the view is not a comparative view
  /// then the panel will be disabled, otherwise, it shows the properties of the
  /// view.
  void setView(pqView*);

  /// Update the view using the current panel values.
  void updateView(); 

  void modeChanged(const QString&);
protected slots:
  /// When the selection of the property to animate on X axis changes.
  void xpropertyChanged();
  
  /// When the selection of the property to animate on Y axis changes.
  void ypropertyChanged();

  /// If vtkSMProxy has a TimestepValues property then this method will set the
  /// TimeRange property of vtkSMComparativeViewProxy to reflect the values.
  void setTimeRangeFromSource(vtkSMProxy*);

protected:
  void activateCue(vtkSMProperty* cuesProperty, 
  vtkSMProxy* animatedProxy, const QString& animatedPName, int animatedIndex);
private:
  pqComparativeVisPanel(const pqComparativeVisPanel&); // Not implemented.
  void operator=(const pqComparativeVisPanel&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif


