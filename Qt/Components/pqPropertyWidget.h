/*=========================================================================

   Program: ParaView
   Module: pqPropertyWidget.h

   Copyright (c) 2005-2012 Sandia Corporation, Kitware Inc.
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

#ifndef _pqPropertyWidget_h
#define _pqPropertyWidget_h

#include "pqComponentsExport.h"

#include <QWidget>

#include "pqPropertyLinks.h"

class pqView;
class vtkSMProxy;
class vtkSMProperty;

class PQCOMPONENTS_EXPORT pqPropertyWidget : public QWidget
{
  Q_OBJECT

public:
  pqPropertyWidget(vtkSMProxy *proxy, QWidget *parent = 0);
  virtual ~pqPropertyWidget();

  virtual void apply();
  virtual void reset();

  pqView* view() const;
  vtkSMProxy* proxy() const;
  vtkSMProperty* property() const;

  bool showLabel() const;

signals:
  /// This signal is emitted when the widget's value is changed by the user.
  void modified();

  /// This signal is emitted when the current view changes.
  void viewChanged(pqView *view);

protected:
  void addPropertyLink(QObject *qobject,
                       const char *qproperty,
                       const char *qsignal,
                       vtkSMProperty *smproperty,
                       int smindex = -1);
  void setShowLabel(bool show);

private:
  void setAutoUpdateVTKObjects(bool autoUpdate);
  void setUseUncheckedProperties(bool useUnchecked);

  friend class pqPropertiesPanel;

private:
  vtkSMProxy *Proxy;
  vtkSMProperty *Property;
  pqPropertyLinks Links;
  bool ShowLabel;
};

#endif // _pqPropertyWidget_h
