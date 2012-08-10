/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiProcessControllerHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiProcessControllerHelper.h"

#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"

vtkStandardNewMacro(vtkMultiProcessControllerHelper);
//----------------------------------------------------------------------------
vtkMultiProcessControllerHelper::vtkMultiProcessControllerHelper()
{
}

//----------------------------------------------------------------------------
vtkMultiProcessControllerHelper::~vtkMultiProcessControllerHelper()
{
}

//----------------------------------------------------------------------------
int vtkMultiProcessControllerHelper::ReduceToAll(
  vtkMultiProcessController* controller,
  vtkMultiProcessStream& data, 
  void (*operation)(vtkMultiProcessStream& A, vtkMultiProcessStream& B),
  int tag)
{
  int myid = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();
  int children[2] = {2*myid + 1, 2*myid + 2};
  int parent = myid > 0? (myid-1)/2 : -1;
  int childno = 0;

  for (childno = 0; childno < 2; childno++)
    {
    int childid = children[childno];
    if (childid >= numProcs)
      {
      // skip nonexistent children.
      continue;
      }
  
    vtkMultiProcessStream child_stream;
    controller->Receive(child_stream, childid, tag);
    (*operation)(child_stream, data);
    }

  if (parent >= 0)
    {
    controller->Send(data, parent, tag);
    data.Reset();
    controller->Receive(data, parent, tag);
    }

  for (childno = 0; childno < 2; childno++)
    {
    int childid = children[childno];
    if (childid >= numProcs)
      {
      // skip nonexistent children.
      continue;
      }
    controller->Send(data, childid, tag); 
    }
  return 1;
}


//----------------------------------------------------------------------------
void vtkMultiProcessControllerHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


