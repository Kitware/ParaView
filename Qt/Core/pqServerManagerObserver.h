/*=========================================================================

   Program: ParaView
   Module:    pqServerManagerObserver.h

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

#ifndef _pqServerManagerObserver_h
#define _pqServerManagerObserver_h

#include "pqCoreModule.h"
#include "vtkType.h" // needed for vtkIdType
#include <QObject>

class pqMultiView;
class pqServerManagerObserverInternal;
class vtkCommand;
class vtkObject;
class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMProxyLocator;

/**
* This is a vtkSMProxyManager observer. This class should simply listen to events
* fired by proxy manager and responds. It does not support any creation method.
* Use pqObjectBuilder for creating objects. The purpose of this class
* is mostly to filter vtkSMProxyManager manager events and emit Qt signals.
*/
class PQCORE_EXPORT pqServerManagerObserver : public QObject
{
  Q_OBJECT

public:
  pqServerManagerObserver(QObject* parent = 0);
  ~pqServerManagerObserver() override;

Q_SIGNALS:
  /**
  * Fired when a compound proxy definition is registered.
  */
  void compoundProxyDefinitionRegistered(QString name);

  /**
  * Fired when a compound proxy definition is unregistered.
  */
  void compoundProxyDefinitionUnRegistered(QString name);

  // Fired when a proxy is registered.
  void proxyRegistered(const QString& group, const QString& name, vtkSMProxy* proxy);

  // Fired when a proxy is unregistered.
  void proxyUnRegistered(const QString& group, const QString& name, vtkSMProxy* proxy);

  /**
  * Fired when a server connection is created by the vtkProcessModule.
  */
  void connectionCreated(vtkIdType connectionId);

  /**
  * Fired when a server connection is closed by  the vtkProcessModule.
  */
  void connectionClosed(vtkIdType connectionId);

  /**
  * Fired when a state file is loaded successfully.
  */
  void stateLoaded(vtkPVXMLElement* root, vtkSMProxyLocator* locator);

  /**
  * Fired when state is being saved.
  */
  void stateSaved(vtkPVXMLElement* root);

private Q_SLOTS:
  void proxyRegistered(
    vtkObject* object, unsigned long e, void* clientData, void* callData, vtkCommand* command);
  void proxyUnRegistered(vtkObject*, unsigned long, void*, void* callData, vtkCommand*);
  void connectionCreated(vtkObject*, unsigned long, void*, void* callData);
  void connectionClosed(vtkObject*, unsigned long, void*, void* callData);
  void stateLoaded(vtkObject*, unsigned long, void*, void* callData);
  void stateSaved(vtkObject*, unsigned long, void*, void* callData);

protected:
  pqServerManagerObserverInternal* Internal; ///< Stores the pipeline objects.
};

#endif // _pqServerManagerObserver_h
