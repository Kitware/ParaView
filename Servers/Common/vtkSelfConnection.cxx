/*=========================================================================

  Program:   ParaView
  Module:    vtkSelfConnection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelfConnection.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkDummyController.h"
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVInformation.h"

vtkStandardNewMacro(vtkSelfConnection);
vtkCxxRevisionMacro(vtkSelfConnection, "1.1");
//-----------------------------------------------------------------------------
vtkSelfConnection::vtkSelfConnection()
{
  this->Controller = vtkDummyController::New();
  vtkMultiProcessController::SetGlobalController(this->Controller);
}

//-----------------------------------------------------------------------------
vtkSelfConnection::~vtkSelfConnection()
{
  vtkMultiProcessController::SetGlobalController(0);
}

//-----------------------------------------------------------------------------
int vtkSelfConnection::Initialize(int argc, char** argv)
{
  this->Controller->Initialize(&argc, &argv, 1);
  // Nothing to do here, really.
  // Just return success.
  return 0;
}

//-----------------------------------------------------------------------------
vtkTypeUInt32 vtkSelfConnection::CreateSendFlag(vtkTypeUInt32 servers)
{
  // Everything is just processed on this single process.
  if (servers != 0)
    {
    return vtkProcessModule::DATA_SERVER_ROOT;
    }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkSelfConnection::SendStreamToDataServerRoot(vtkClientServerStream& stream)
{
  return this->ProcessStreamLocally(stream);
}

//-----------------------------------------------------------------------------
int vtkSelfConnection::ProcessStreamLocally(vtkClientServerStream& stream)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->GetInterpreter()->ProcessStream(stream);
  return 0;
}

//-----------------------------------------------------------------------------
int vtkSelfConnection::ProcessStreamLocally(unsigned char* data, int length)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->GetInterpreter()->ProcessStream(data, length);
  return 0;
}

//-----------------------------------------------------------------------------
const vtkClientServerStream& vtkSelfConnection::GetLastResult(vtkTypeUInt32 )
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  return pm->GetInterpreter()->GetLastResult();
}
  
//-----------------------------------------------------------------------------
void vtkSelfConnection::GatherInformation(vtkTypeUInt32 vtkNotUsed(serverFlags), 
  vtkPVInformation* info, vtkClientServerID id)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkObject* object = vtkObject::SafeDownCast(pm->GetObjectFromID(id));
  if (!object)
    {
    vtkErrorMacro("Failed to locate object with ID: " << id);
    return;
    }

  info->CopyFromObject(object);
}

//-----------------------------------------------------------------------------
int vtkSelfConnection::LoadModule(const char* name, const char* directory)
{
  const char* paths[] = { directory, 0};
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int localResult = pm->GetInterpreter()->Load(name, paths);
  return localResult;
}

//-----------------------------------------------------------------------------
void vtkSelfConnection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
