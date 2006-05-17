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
#include <QObject>
#include "vtkType.h" // needed for vtkIdType
class pqMultiView;
class pqPipelineDataInternal;
class pqPipelineModel;

class QVTKWidget;

class vtkCommand;
class vtkObject;
class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMRenderModuleProxy;

// This is a vtkSMProxyManager observer. This class should simply listen to events
// fired by proxy manager and respond. It does not support any creation method. 
// We must use pqPipelineBuilder for that purpose. The purpose of this class
// is mostly to filter vtkSMProxyManager manager events and emit Qt signals.
class PQWIDGETS_EXPORT pqPipelineData : public QObject
{
  Q_OBJECT

public:
  pqPipelineData(QObject* parent=0);
  virtual ~pqPipelineData();

signals:
  /// Fired when a proxy is registered under the "sources" group and 
  /// belongs to "sources" xml group.. 
  void sourceRegistered(QString name, vtkSMProxy *source);

  /// Fired when a proxy is registered under the "sources" group and
  /// belongs to "filters" xml group.
  void filterRegistered(QString name, vtkSMProxy *filter);

  /// Fired when a compound proxy is registered.
  void bundleRegistered(QString name, vtkSMProxy *bundle);

  /// Fired when a proxy is registered under the "displays" group.
  void displayRegistered(QString name, vtkSMProxy* display);

  // Fired when a proxy registered under group "displays" is unregistered.
  void displayUnRegistered(vtkSMProxy* display);

  /// Fired when a source/filter/compound proxy is unregistered.
  void proxyUnRegistered(vtkSMProxy *proxy);

  /// Fired when a server connection is created by the vtkProcessModule.
  void connectionCreated(vtkIdType connectionId);

  /// Fired when a server connection is closed by  the vtkProcessModule.
  void connectionClosed(vtkIdType connectionId);

  /// Fired when a render module proxy is registered under the "render_modules"
  /// group.
  void renderModuleRegistered(QString name, vtkSMRenderModuleProxy* rm);

  /// Fired when a render module proxy is unregistred.
  void renderModuleUnRegistered(vtkSMRenderModuleProxy* rm);

private slots:
  void proxyRegistered(vtkObject* object, unsigned long e, void* clientData,
      void* callData, vtkCommand* command);
  void proxyUnRegistered(vtkObject*, unsigned long, void*,
    void* callData, vtkCommand*);
  void connectionCreated(vtkObject*, unsigned long, void*, void* callData);
  void connectionClosed(vtkObject*, unsigned long, void*, void* callData);

protected:
  pqPipelineDataInternal *Internal;  ///< Stores the pipeline objects.
};

#endif // _pqPipelineData_h

