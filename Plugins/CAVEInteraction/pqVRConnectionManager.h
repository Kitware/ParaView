// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqVRConnectionManager_h
#define pqVRConnectionManager_h
#include "vtkPVVRConfig.h"

#include <QObject>
#include <QPointer>
class vtkVRQueue;
class vtkPVXMLElement;
class vtkSMProxyLocator;
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
class pqVRUIConnection;
#endif
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
class pqVRPNConnection;
#endif

class pqVRConnectionManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqVRConnectionManager(vtkVRQueue* queue, QObject* parent = 0);
  virtual ~pqVRConnectionManager();
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
  void add(pqVRPNConnection* conn);
  void remove(pqVRPNConnection* conn);
  pqVRPNConnection* GetVRPNConnection(const QString& name);
#endif
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
  void add(pqVRUIConnection* conn);
  void remove(pqVRUIConnection* conn);
  pqVRUIConnection* GetVRUIConnection(const QString& name);
#endif
  void clear();

  QList<QString> connectionNames() const;

  int numConnections() const;

  static pqVRConnectionManager* instance();

public Q_SLOTS:
  /// start/stop connections
  void start();
  void stop();

  /// Clears current connections and loads a new set of connections from the XML
  /// Configuration
  void configureConnections(vtkPVXMLElement* xml, vtkSMProxyLocator* locator);

  // save the connection configuration
  void saveConnectionsConfiguration(vtkPVXMLElement* root);

Q_SIGNALS:
  void connectionsChanged();

private:
  Q_DISABLE_COPY(pqVRConnectionManager)
  class pqInternals;
  pqInternals* Internals;

  friend class pqVRStarter;
  static void setInstance(pqVRConnectionManager*);
  static QPointer<pqVRConnectionManager> Instance;
};

#endif // pqVRConnectionManager_h
