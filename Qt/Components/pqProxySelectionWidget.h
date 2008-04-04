/*=========================================================================

   Program:   ParaView
   Module:    pqProxySelectionWidget.h

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
#ifndef __pqProxySelectionWidget_h
#define __pqProxySelectionWidget_h

#include <QWidget>
#include <QVariant>
#include "pqComponentsExport.h"
#include "pqSMProxy.h"

class vtkSMProxy;
class vtkSMProperty;
class pqView;

/// a widget that contains a combo box and child widget
/// for represents a ProxyProperty with a ProxyListDomain
/// the combo box selects the proxy, and the child widget
/// contains properties of the proxy
class PQCOMPONENTS_EXPORT pqProxySelectionWidget : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(pqSMProxy proxy READ proxy WRITE setProxy)
public:
  /// constructor requires the proxy, property
  pqProxySelectionWidget(vtkSMProxy* proxy, const QString& property,
                    const QString& label = QString(), 
                    QWidget* parent = NULL);
  ~pqProxySelectionWidget();

  /// get the selected proxy
  pqSMProxy proxy() const;
  
signals:
  /// signal the proxy changed (QVariant wrapped pqSMProxy)
  void proxyChanged(pqSMProxy);

  void modified();

public slots:
  /// set the proxy
  virtual void setProxy(pqSMProxy);

  /// Activates the widget
  /// slot is forwarded to the sub panel
  virtual void select();

  /// Deactivates the widget
  /// slot is forwarded to the sub panel
  virtual void deselect();

  /// Accepts pending changes.
  /// slot is forwarded to the sub panel
  virtual void accept();

  /// Resets pending changes
  /// slot is forwarded to the sub panel
  virtual void reset();

  /// Set the render module that this widget works with
  /// slot is forwarded to the sub panel
  virtual void setView(pqView* rm);

protected slots:
  void handleProxyChanged();

private:
  void initialize3DWidget();
  class pqInternal;
  pqInternal* Internal;
};

#endif

