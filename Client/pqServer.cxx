/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqServer.h"

#include <vtkSMApplication.h>

namespace
{

vtkSMApplication* g_server = 0;

} // namespace

vtkSMApplication* GetServer()
{
  if(!g_server)
    {
    g_server = vtkSMApplication::New();
    g_server->Initialize();
    }
    
   return g_server;
}

void FinalizeServer()
{
  if(!g_server)
    return;

  g_server->Finalize();
  g_server->Delete();
  g_server = 0;    
}

