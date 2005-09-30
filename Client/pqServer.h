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

class vtkProcessModule;
class vtkSMProxyManager;
class vtkSMRenderModuleProxy;

class pqServer
{
public:
  pqServer();
  ~pqServer();

  vtkProcessModule* GetProcessModule();
  vtkSMProxyManager* GetProxyManager();
  vtkSMRenderModuleProxy* GetRenderModule();
  
private:
  class implementation;
  implementation* const Implementation;
};

#endif // !_pqServer_h

