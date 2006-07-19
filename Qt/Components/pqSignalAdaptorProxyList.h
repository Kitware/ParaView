/*=========================================================================

   Program:   ParaView
   Module:    pqSignalAdaptorProxyList.h

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
#ifndef __pqSignalAdaptorProxyList_h
#define __pqSignalAdaptorProxyList_h

#include <QObject>
#include <QVariant>
#include "pqComponentsExport.h"

class pq3DWidget;
class pqProxy;
class pqRenderModule;
class pqSignalAdaptorProxyListInternal;
class QComboBox;
class QWidget;
class vtkSMProperty;
class vtkSMProxy;

/// signal adaptor to allow getting/setting/observing of a pseudo vtkSMProxy property
/// which has a proxy list domain.
/// pqSignalAdaptorProxyList must be used for any combo-box that selects among
/// proxies, any of which can have a 3D widget. The 3D widget information is obtained
/// from the server manager hints for the selected proxy.
class PQCOMPONENTS_EXPORT pqSignalAdaptorProxyList : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QVariant proxy READ proxy WRITE setProxy)
public:
  /// constructor requires a QObject, the name of the QString proxy name, 
  /// and a signal for property changes.
  pqSignalAdaptorProxyList(QComboBox* p, pqProxy* proxy, const char* smproperty_name);
  ~pqSignalAdaptorProxyList();

  /// get the selected proxy.
  QVariant proxy() const;

  /// Set the frame in which this adaptor can put the 3D widget, if needed.
  void setWidgetFrame(QWidget*);
signals:
  /// signal the proxy changed
  void proxyChanged(const QVariant&);

  /// signal fired when the widget becomes modified. eg. when the 3D widget
  /// for some proxy is the list if modified.
  void modified();

public slots:
  /// set the proxy
  void setProxy(const QVariant&);

  /// Set the render module.
  void setRenderModule(pqRenderModule*);

  // These slots must be connected to the corresponding signals from the 
  // panel. Unlike most other signal adaptors that don't care about the
  // panel, this adaptor is relies on these signals from the panel,
  // since the 3D widget visibility etc depends on it.
  void select();
  void deselect();
  void accept();
  void reset();
protected slots:
  void handleProxyChanged();

  /// This is a convenience method. 
  /// Given a \c proxy, this method checks the \c hints, if any, on the proxy
  /// and setups up a suitable pq3DWidget subclass.
  pq3DWidget* new3DWidget(vtkSMProxy* proxy);

protected:
  QByteArray PropertyName;
  void initialize3DWidget();

  void createProxyList();
private:
  pqSignalAdaptorProxyListInternal* Internal;
};

#endif

