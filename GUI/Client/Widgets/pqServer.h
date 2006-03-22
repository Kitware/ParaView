/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqServer_h
#define _pqServer_h

class pqOptions;
class vtkProcessModule;
class vtkSMApplication;
class vtkSMProxyManager;
class vtkSMMultiViewRenderModuleProxy;

#include "QtWidgetsExport.h"
#include <QObject>
#include <QString>

/// Abstracts the concept of a "server connection" so that ParaQ clients may: have more than one connect at a time / open and close connections at-will
class QTWIDGETS_EXPORT pqServer : public QObject
{
public:
  /// Constructs a standalone or "built-in" server connection, returns NULL on failure
  static pqServer* CreateStandalone();
  /// Constructs a server connection to a remote host, returns NULL on failure
  static pqServer* CreateConnection(const char* const hostName, const int portNumber);
  virtual ~pqServer();

  QString getAddress() const;
  const QString& getFriendlyName() const {return this->FriendlyName;}
  void setFriendlyName(const QString& name);

  vtkProcessModule* GetProcessModule();
  static vtkSMProxyManager* GetProxyManager();
  vtkSMMultiViewRenderModuleProxy* GetRenderModule();

private:
  pqServer(pqOptions*, vtkProcessModule*, vtkSMApplication*, vtkSMMultiViewRenderModuleProxy*);
  pqServer(const pqServer&);
  pqServer& operator=(const pqServer&);

  QString FriendlyName;
  pqOptions* const Options;
  vtkProcessModule* const ProcessModule;
  static vtkSMApplication* ServerManager;
  vtkSMMultiViewRenderModuleProxy* const RenderModule;
};

#endif // !_pqServer_h

