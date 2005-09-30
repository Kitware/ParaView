// -*- c++ -*-

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqCamera.h"
#include "pqServer.h"

#include <vtkSMRenderModuleProxy.h>

void pqResetCamera(pqServer& Server)
{
  double bounds[6];
  Server.GetRenderModule()->ComputeVisiblePropBounds(bounds);
  
  if (   (bounds[0] <= bounds[1])
      && (bounds[2] <= bounds[3])
      && (bounds[4] <= bounds[5]) )
    {
    Server.GetRenderModule()->ResetCamera(bounds);
    }
}
