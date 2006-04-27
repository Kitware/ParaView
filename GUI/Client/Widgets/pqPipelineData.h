/*=========================================================================

   Program:   ParaQ
   Module:    pqPipelineData.h

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

#ifndef _pqPipelineData_h
#define _pqPipelineData_h

#include "pqWidgetsExport.h"
#include <vtkSmartPointer.h>
#include <QObject>

class pqMultiView;
class pqNameCount;
class pqPipelineDataInternal;
class pqPipelineModel;
class pqServer;

class QString;
class QVTKWidget;

class vtkCommand;
class vtkEventQtSlotConnect;
class vtkObject;
class vtkPVXMLElement;
class vtkSMDisplayProxy;
class vtkSMProxy;
class vtkSMRenderModuleProxy;


/// interface for querying pipline state, also provides signals for pipeline changes
class PQWIDGETS_EXPORT pqPipelineData : public QObject
{
  Q_OBJECT

public:
  pqPipelineData(QObject* parent=0);
  virtual ~pqPipelineData();

  static pqPipelineData *instance();

  pqPipelineModel *getModel() const {return this->Model;}
  pqNameCount *getNameCount() const {return this->Names;}

  void addServer(pqServer *server);
  void removeServer(pqServer *server);

  void addViewMapping(QVTKWidget *widget, vtkSMRenderModuleProxy *module);
  vtkSMRenderModuleProxy *removeViewMapping(QVTKWidget *widget);
  vtkSMRenderModuleProxy *getRenderModule(QVTKWidget *widget) const;
  QVTKWidget *getRenderWindow(vtkSMRenderModuleProxy *module) const;
  void clearViewMapping();

  vtkSMProxy *createAndRegisterSource(const char *proxyName, pqServer *server);
  vtkSMProxy *createAndRegisterFilter(const char *proxyName, pqServer *server);
  vtkSMProxy *createAndRegisterBundle(const char *proxyName, pqServer *server);

  void unregisterProxy(vtkSMProxy *proxy, const char *name);

  /// \brief
  ///   Creates a display proxy for a source proxy.
  vtkSMDisplayProxy *createAndRegisterDisplay(vtkSMProxy *source,
      vtkSMRenderModuleProxy *module);
  void removeAndUnregisterDisplay(vtkSMDisplayProxy *display,
      const char *name, vtkSMRenderModuleProxy *module);

  /// \brief
  ///   Sets whether the display proxy is visible or not.
  void setVisible(vtkSMDisplayProxy *proxy, bool visible);

  void addInput(vtkSMProxy *proxy, vtkSMProxy *input);
  void removeInput(vtkSMProxy *proxy, vtkSMProxy *input);
  void addConnection(vtkSMProxy *source, vtkSMProxy *sink);
  void removeConnection(vtkSMProxy *source, vtkSMProxy *sink);

  void loadState(vtkPVXMLElement *root, pqMultiView *multiView=0);

signals:
  void serverAdded(pqServer *server);
  void removingServer(pqServer *server);

  void sourceCreated(vtkSMProxy *source, const QString &name, pqServer *server);
  void filterCreated(vtkSMProxy *filter, const QString &name, pqServer *server);
  void bundleCreated(vtkSMProxy *bundle, const QString &name, pqServer *server);
  void removingProxy(vtkSMProxy *proxy);

  void displayCreated(vtkSMDisplayProxy *display, const QString &name,
      vtkSMProxy *proxy, vtkSMRenderModuleProxy *module);
  void removingDisplay(vtkSMDisplayProxy *display, vtkSMProxy *proxy);

  void connectionCreated(vtkSMProxy *source, vtkSMProxy *sink);
  void removingConnection(vtkSMProxy *source, vtkSMProxy *sink);

  /// new current proxy
  void currentProxyChanged(vtkSMProxy* newproxy);

public slots:
  /// set the current proxy
  void setCurrentProxy(vtkSMProxy* proxy);

public:
  /// get the current proxy
  vtkSMProxy* currentProxy() const;

private slots:
  void proxyRegistered(vtkObject* object, unsigned long e, void* clientData,
      void* callData, vtkCommand* command);
  void inputChanged(vtkObject* object, unsigned long e, void* clientData,
      void* callData, vtkCommand* command);

protected:
  pqPipelineDataInternal *Internal;  ///< Stores the pipeline objects.
  pqPipelineModel *Model;            ///< View interface to the pipeline.
  pqNameCount *Names;                ///< Keeps track of the proxy names.
  bool InCreateOrConnect;            ///< Used to prevent duplicate signals.

  /// Used to listen to proxy events.
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  vtkSMProxy* CurrentProxy;

  static pqPipelineData *Instance;   ///< Pointer to the pipeline instance.
};

#endif // _pqPipelineData_h

