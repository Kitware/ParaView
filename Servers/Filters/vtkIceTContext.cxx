/*=========================================================================

  Program:   ParaView
  Module:    vtkIceTContext.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkIceTContext.h"

#include "vtkMPI.h"
#include "vtkMPIController.h"
#include "vtkMPICommunicator.h"
#include "vtkObjectFactory.h"

#include "GL/ice-t.h"
#include "GL/ice-t_mpi.h"

//-----------------------------------------------------------------------------

class vtkIceTContextOpaqueHandle
{
public:
  IceTContext Handle;
};

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkIceTContext);

vtkIceTContext::vtkIceTContext()
{
  this->Controller = NULL;

  this->Context = new vtkIceTContextOpaqueHandle;
}

vtkIceTContext::~vtkIceTContext()
{
  this->SetController(NULL);

  delete this->Context;
}

void vtkIceTContext::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Controller: " << this->Controller << endl;
}

//-----------------------------------------------------------------------------

void vtkIceTContext::SetController(vtkMultiProcessController *controller)
{
  if (controller == this->Controller)
    {
    return;
    }

  IceTContext newContext = (IceTContext)-1;

  if (controller)
    {
    vtkMPICommunicator *communicator
      = vtkMPICommunicator::SafeDownCast(controller->GetCommunicator());
    if (!communicator)
      {
      vtkErrorMacro("IceT can currently be only used with an MPI communicator.");
      return;
      }

    MPI_Comm mpiComm = *communicator->GetMPIComm()->GetHandle();
    IceTCommunicator icetComm = icetCreateMPICommunicator(mpiComm);
    newContext = icetCreateContext(icetComm);
    icetDestroyMPICommunicator(icetComm);

    if (this->Controller)
      {
      icetCopyState(newContext, this->Context->Handle);
      }
    }

  if (this->Controller)
    {
    icetDestroyContext(this->Context->Handle);
    this->Controller->UnRegister(this);
    }

  this->Controller = controller;
  this->Context->Handle = newContext;

  if (this->Controller)
    {
    this->Controller->Register(this);
    }

  this->Modified();
}

//-----------------------------------------------------------------------------

void vtkIceTContext::MakeCurrent()
{
  if (!this->Controller)
    {
    vtkErrorMacro("Must set controller before making an IceT context current.");
    return;
    }

  icetSetContext(this->Context->Handle);
}

//-----------------------------------------------------------------------------

void vtkIceTContext::CopyState(vtkIceTContext *src)
{
  icetCopyState(this->Context->Handle, src->Context->Handle);
}

//-----------------------------------------------------------------------------

int vtkIceTContext::IsValid()
{
  return (this->Controller != NULL);
}
