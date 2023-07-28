// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqServerManagerObserver_h
#define pqServerManagerObserver_h

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
  pqServerManagerObserver(QObject* parent = nullptr);
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

#endif // pqServerManagerObserver_h
