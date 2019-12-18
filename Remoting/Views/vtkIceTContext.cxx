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
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkObjectFactory.h"

#include "IceT.h"
#include "IceTGL.h"
#include "IceTMPI.h"

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
  // This class establishes a constraint that these are both NULL or both valid.
  this->Controller = NULL;
  this->Context = NULL;
  this->UseOpenGL = 0;
}

vtkIceTContext::~vtkIceTContext()
{
  // Class constraint dictates that the context will be deleted as well.
  this->SetController(NULL);
}

void vtkIceTContext::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Controller: " << this->Controller << endl;
}

//-----------------------------------------------------------------------------

void vtkIceTContext::SetController(vtkMultiProcessController* controller)
{
  if (controller == this->Controller)
  {
    return;
  }

  vtkIceTContextOpaqueHandle* newContext = NULL;

  if (controller)
  {
    vtkMPICommunicator* communicator =
      vtkMPICommunicator::SafeDownCast(controller->GetCommunicator());
    if (!communicator)
    {
      vtkErrorMacro("IceT can currently be only used with an MPI communicator.");
      return;
    }

    MPI_Comm mpiComm = *communicator->GetMPIComm()->GetHandle();
    IceTCommunicator icetComm = icetCreateMPICommunicator(mpiComm);
    newContext = new vtkIceTContextOpaqueHandle;
    newContext->Handle = icetCreateContext(icetComm);
    icetDestroyMPICommunicator(icetComm);

    if (this->UseOpenGL)
    {
      icetGLInitialize();
    }

    if (this->IsValid())
    {
      icetCopyState(newContext->Handle, this->Context->Handle);
    }
  }

  if (this->Controller)
  {
    icetDestroyContext(this->Context->Handle);
    delete this->Context;
    this->Context = NULL;
    this->Controller->UnRegister(this);
    this->Controller = NULL;
  }

  this->Controller = controller;
  this->Context = newContext;

  if (this->Controller)
  {
    this->Controller->Register(this);
  }

  this->Modified();
}

//-----------------------------------------------------------------------------

void vtkIceTContext::MakeCurrent()
{
  if (!this->IsValid())
  {
    vtkErrorMacro("Must set controller before making an IceT context current.");
    return;
  }

  icetSetContext(this->Context->Handle);
}

//-----------------------------------------------------------------------------

void vtkIceTContext::SetUseOpenGL(int flag)
{
  if (this->UseOpenGL == flag)
    return;

  this->UseOpenGL = flag;
  this->Modified();

  if (this->UseOpenGL && this->IsValid())
  {
    this->MakeCurrent();
    if (!icetGLIsInitialized())
    {
      icetGLInitialize();
    }
  }
}

//-----------------------------------------------------------------------------

void vtkIceTContext::CopyState(vtkIceTContext* src)
{
  if (!this->IsValid())
  {
    vtkErrorMacro("Must set controller to copy state to context.");
    return;
  }
  if (!src->IsValid())
  {
    vtkErrorMacro("Must set controller to copy state from context.");
    return;
  }

  icetCopyState(this->Context->Handle, src->Context->Handle);
}

//-----------------------------------------------------------------------------

int vtkIceTContext::IsValid()
{
  return ((this->Controller != NULL) && (this->Context != NULL));
}
