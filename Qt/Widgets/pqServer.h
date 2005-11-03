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
class vtkSMRenderModuleProxy;

#include <string>

/// Abstracts the concept of a "server connection" so that ParaQ clients may: have more than one connect at a time / open and close connections at-will
class pqServer
{
public:
  /// Constructs a standalone or "built-in" server connection, returns NULL on failure
  static pqServer* Standalone();
  /// Constructs a server connection to a remote host, returns NULL on failure
  static pqServer* Connect(const char* const hostName, const int portNumber);
  virtual ~pqServer();

  vtkProcessModule* GetProcessModule();
  vtkSMProxyManager* GetProxyManager();
  vtkSMRenderModuleProxy* GetRenderModule();

private:
  pqServer(pqOptions*, vtkProcessModule*, vtkSMApplication*, vtkSMRenderModuleProxy*);
  pqServer(const pqServer&);
  pqServer& operator=(const pqServer&);
  
  pqOptions* const Options;
  vtkProcessModule* const ProcessModule;
  vtkSMApplication* const ServerManager;
  vtkSMRenderModuleProxy* const RenderModule;
};

#endif // !_pqServer_h

