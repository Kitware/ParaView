/*=========================================================================

  Program:   ParaView
  Module:    vtkMPICompositeManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMPICompositeManager.h"

#include "vtkObjectFactory.h"
#include "vtkByteSwap.h"
#include "vtkMultiProcessController.h"
#include "vtkRenderWindow.h"
vtkStandardNewMacro(vtkMPICompositeManager);
vtkCxxRevisionMacro(vtkMPICompositeManager, "1.1");

//-----------------------------------------------------------------------------
static void vtkMPICompositeManagerGatherZBufferValueRMI(void *local, void *pArg, 
                                                    int pLength, int)
{
  vtkMPICompositeManager* self = (vtkMPICompositeManager*)local;
  int *p;
  int x, y;

  if (pLength != sizeof(int)*3)
    {
    vtkGenericWarningMacro("Integer sizes differ.");
    }

  p = (int*)pArg;
  if (p[0] != 1)
    { // Need to swap
    vtkByteSwap::SwapVoidRange(pArg, 3, sizeof(int));
    if (p[0] != 1)
      { // Swapping did not work.
      vtkGenericWarningMacro("Swapping failed.");
      }
    }
  x = p[1];
  y = p[2];
  self->GatherZBufferValueRMI(x, y);
}

//-----------------------------------------------------------------------------
vtkMPICompositeManager::vtkMPICompositeManager()
{

}

//-----------------------------------------------------------------------------
vtkMPICompositeManager::~vtkMPICompositeManager()
{

}

//-----------------------------------------------------------------------------
// Called only on Root node.
float vtkMPICompositeManager::GetZBufferValue(int x, int y)
{
  float z;
  int pArg[3];
  float *pz;
  pz = this->RenderWindow->GetZbufferData(x, y, x, y);
  z = *pz;
  delete [] pz;
  if (this->UseCompositing == 0 || !this->Controller)
    {
    // This could cause a problem between setting this ivar and rendering.
    // We could always composite, and always consider client z.
    return z; // no need to collect from other processes.
    }
  
  int myId = this->Controller->GetLocalProcessId();
  if (myId != 0)
    {
    vtkErrorMacro("GetZBufferValue must be called only on Root Node.");
    return 0;
    }
  
  float otherZ;
  int numProcs = this->Controller->GetNumberOfProcesses();
  int idx;
  pArg[0] = 1;
  pArg[1] = x;
  pArg[2] = y;

  for (idx = 1; idx < numProcs; ++idx)
    {
    // Request the Z from all other processes.
    this->Controller->TriggerRMI(1, (void*)pArg, sizeof(int)*3, 
      vtkMPICompositeManager::GATHER_Z_RMI_TAG);
    }
  for (idx = 1; idx < numProcs; ++idx)
    {
    // Receive the Z from all other processes and find the minimum.
    this->Controller->Receive(&otherZ, 1, idx, vtkMPICompositeManager::Z_TAG);
    if (otherZ < z)
      {
      z = otherZ;
      }
    }
  return z;
}
//----------------------------------------------------------------------------
// Get called on every process other than Root Node.
void vtkMPICompositeManager::GatherZBufferValueRMI(int x, int y)
{
  float z;

  // Get the z value.
  int *size = this->RenderWindow->GetSize();
  if (x < 0 || x >= size[0] || y < 0 || y >= size[1])
    {
    vtkErrorMacro("Point not contained in window.");
    z = 0;
    }
  else
    {
    float *tmp;
    tmp = this->RenderWindow->GetZbufferData(x, y, x, y);
    z = *tmp;
    delete [] tmp;
    }

  int myId = this->Controller->GetLocalProcessId();
  if (myId == 0)
    {
    vtkErrorMacro("This method should not have gotten called on node 0!!!");
    return;
    }
  else
    {
    // Send z to the root server node (ie. node 0)..
    this->Controller->Send(&z, 1, 1, vtkMPICompositeManager::Z_TAG);
    }
}

//-----------------------------------------------------------------------------
void vtkMPICompositeManager::InitializeRMIs()
{
  if (!this->Controller)
    {
    vtkErrorMacro("Missing Controller!");
    return;
    }
  if (this->Controller->GetLocalProcessId() == 0)
    {
    // Root node, does not need to listen to any RMI triggers.
    return;
    }
  this->Superclass::InitializeRMIs();
  
  this->Controller->AddRMI(::vtkMPICompositeManagerGatherZBufferValueRMI, this,
    vtkMPICompositeManager::GATHER_Z_RMI_TAG);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void vtkMPICompositeManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
