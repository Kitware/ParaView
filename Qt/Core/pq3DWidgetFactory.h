/*=========================================================================

   Program: ParaView
   Module:    pq3DWidgetFactory.h

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
#ifndef __pq3DWidgetFactory_h
#define __pq3DWidgetFactory_h

#include "pqCoreExport.h"
#include <QObject>

class pq3DWidgetFactoryInternal;
class pqServer;

class vtkSMNewWidgetRepresentationProxy;
class vtkSMProxy;

/// This is 3DWidget Factory. One can request a 3D widget of any type
/// from this class on the given server. If a 3D widget is available,
/// it will be reused, and marked \c in-use. When the caller is done
/// using the 3D widget, it must call free3DWidget(), which makes 
/// the widget available for reuse.
class PQCORE_EXPORT pq3DWidgetFactory : public QObject
{
  Q_OBJECT
public:
  pq3DWidgetFactory(QObject* parent=NULL);
  virtual ~pq3DWidgetFactory();
 
  /// Use this method to obtain a 3D widget of the given
  /// \c name on the given \c server. If no 3D widget is available,
  /// a new one will be created on the server and returned. The caller
  /// can call free3DWidget() when it is done with the 3D Widget, that 
  /// way it can be used by others.
  vtkSMNewWidgetRepresentationProxy* get3DWidget(const QString& name,
    pqServer* server);

  /// Call this method when done with using a 3D widget, this makes
  /// the 3DWidget available for reuse.
  void free3DWidget(vtkSMNewWidgetRepresentationProxy* widget);

private slots:
  /// When the 3D widget proxy is unregistered, we must release the internal
  /// reference as well. Eventually, once this class starts using pqProxy
  /// objects to manage the 3D widget proxies, the pqProxy objects will
  /// deleted by the pqServerManagerModel, thus letting go for the vtkSMProxy
  /// reference. This class will keep QPointers to the pqProxy objects,
  /// hence there will be no dangling pointers. But until then, just
  /// notice every unregister event and do the cleanup.
  void proxyUnRegistered(QString group, QString name, vtkSMProxy* proxy);

protected:
  pq3DWidgetFactoryInternal* Internal;
};

#endif

