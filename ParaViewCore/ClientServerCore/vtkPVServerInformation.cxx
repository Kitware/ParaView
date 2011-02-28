/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVServerInformation.h"

#include "vtkClientServerStream.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPVServerOptions.h"
#include "vtkPVServerOptionsInternals.h"
#include "vtkToolkits.h"

vtkStandardNewMacro(vtkPVServerInformation);

//----------------------------------------------------------------------------
vtkPVServerInformation::vtkPVServerInformation()
{
  this->NumberOfProcesses = vtkMultiProcessController::GetGlobalController() ?
                            vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses() :
                            1;
  this->RootOnly = 1;
  this->RemoteRendering = 1;
  this->TileDimensions[0] = this->TileDimensions[1] = 0;
  this->TileMullions[0] = this->TileMullions[1] = 0;
  this->UseOffscreenRendering = 0;
  this->Timeout = 0;
#if defined(PARAVIEW_USE_ICE_T) && defined(VTK_USE_MPI)
  this->UseIceT = 1;
#else
  this->UseIceT = 0;
#endif

  this->AVISupport = 0;
#if defined(_WIN32)
  this->AVISupport = 1;
#else 
# if defined(VTK_USE_FFMPEG_ENCODER)
  this->AVISupport = 1;
# endif
#endif
#if defined(VTK_USE_OGGTHEORA_ENCODER)
  this->OGVSupport = 1;
#else
  this->OGVSupport = 0;
#endif

  this->RenderModuleName = NULL;
  this->MachinesInternals = new vtkPVServerOptionsInternals;
}

//----------------------------------------------------------------------------
vtkPVServerInformation::~vtkPVServerInformation()
{
  this->SetRenderModuleName(NULL);
  delete this->MachinesInternals;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RemoteRendering: " << this->RemoteRendering << endl;
  os << indent << "UseOffscreenRendering: " << this->UseOffscreenRendering << endl;
  os << indent << "TileDimensions: " << this->TileDimensions[0]
     << ", " << this->TileDimensions[1] << endl;
  os << indent << "TileMullions: " << this->TileMullions[0]
     << ", " << this->TileMullions[1] << endl;
  os << indent << "UseIceT: " << this->UseIceT << endl;
  os << indent << "RenderModuleName: "
     << (this->RenderModuleName ? this->RenderModuleName : "(none)") << endl;
  os << indent << "OGVSupport: " << this->OGVSupport << endl;
  os << indent << "AVISupport: " << this->AVISupport << endl;
  os << indent << "Timeout: " << this->Timeout << endl;
  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::DeepCopy(vtkPVServerInformation *info)
{
  this->RemoteRendering = info->GetRemoteRendering();
  info->GetTileDimensions(this->TileDimensions);
  info->GetTileMullions(this->TileMullions);
  this->UseOffscreenRendering = info->GetUseOffscreenRendering();
  this->UseIceT = info->GetUseIceT();
  this->SetRenderModuleName(info->GetRenderModuleName());
  this->Timeout = info->GetTimeout();
  this->SetNumberOfMachines(info->GetNumberOfMachines());
  unsigned int idx;
  for (idx = 0; idx < info->GetNumberOfMachines(); idx++)
    {
    this->SetEnvironment(idx, info->GetEnvironment(idx));
    this->SetLowerLeft(idx, info->GetLowerLeft(idx));
    this->SetLowerRight(idx, info->GetLowerRight(idx));
    this->SetUpperRight(idx, info->GetUpperRight(idx));
    }
  this->NumberOfProcesses = info->NumberOfProcesses;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::CopyFromObject(vtkObject* vtkNotUsed(obj))
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if(!pm)
    {
    vtkWarningMacro("ProcessModule is not available.");
    return;
    }

  vtkPVOptions* options = pm->GetOptions();
  vtkPVServerOptions *serverOptions = vtkPVServerOptions::SafeDownCast(options);

  options->GetTileDimensions(this->TileDimensions);
  options->GetTileMullions(this->TileMullions);
#if !defined(__APPLE__)
  this->UseOffscreenRendering = options->GetUseOffscreenRendering();
#else
  this->UseOffscreenRendering = 0;
#endif  
  this->Timeout = options->GetTimeout();
  this->SetRenderModuleName(options->GetRenderModuleName());

  if (serverOptions)
    {
    this->SetNumberOfMachines(serverOptions->GetNumberOfMachines());
    unsigned int idx;
    for (idx = 0; idx < serverOptions->GetNumberOfMachines(); idx++)
      {
      this->SetEnvironment(idx, serverOptions->GetDisplayName(idx));
      this->SetLowerLeft(idx, serverOptions->GetLowerLeft(idx));
      this->SetLowerRight(idx, serverOptions->GetLowerRight(idx));
      this->SetUpperRight(idx, serverOptions->GetUpperRight(idx));
      }
    }
}

//----------------------------------------------------------------------------
// Consider an option added if it is a non-default option that the user
// has probably selected.
void vtkPVServerInformation::AddInformation(vtkPVInformation* info)
{
  vtkPVServerInformation* serverInfo;
  serverInfo = vtkPVServerInformation::SafeDownCast(info);
  if (serverInfo)
    {
    if (!serverInfo->GetRemoteRendering())
      {
      this->RemoteRendering = 0;
      }
    if (serverInfo->GetTileDimensions()[0])
      {
      serverInfo->GetTileDimensions(this->TileDimensions);
      }
    if (serverInfo->GetTileMullions()[0])
      {
      serverInfo->GetTileMullions(this->TileMullions);
      }
    if (serverInfo->GetUseOffscreenRendering())
      {
      this->UseOffscreenRendering = 1;
      }

    if (this->Timeout <= 0 || 
      (serverInfo->GetTimeout() > 0 && serverInfo->GetTimeout() < this->Timeout))
      {
      this->Timeout = serverInfo->GetTimeout();
      }

    if (!serverInfo->GetOGVSupport())
      {
      this->OGVSupport = 0;
      }

    if (!serverInfo->GetAVISupport())
      {
      this->AVISupport = 0;
      }

    // IceT either is there or is not.
    this->UseIceT = serverInfo->GetUseIceT();
    this->SetRenderModuleName(serverInfo->GetRenderModuleName());
    this->SetNumberOfMachines(serverInfo->GetNumberOfMachines());
    unsigned int idx;
    for (idx = 0; idx < serverInfo->GetNumberOfMachines(); idx++)
      {
      this->SetEnvironment(idx, serverInfo->GetEnvironment(idx));
      this->SetLowerLeft(idx, serverInfo->GetLowerLeft(idx));
      this->SetLowerRight(idx, serverInfo->GetLowerRight(idx));
      this->SetUpperRight(idx, serverInfo->GetUpperRight(idx));
      }

    if (this->NumberOfProcesses < serverInfo->NumberOfProcesses)
      {
      this->NumberOfProcesses = serverInfo->NumberOfProcesses;
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;
  *css << this->RemoteRendering;
  *css << this->TileDimensions[0] << this->TileDimensions[1];
  *css << this->TileMullions[0] << this->TileMullions[1];
  *css << this->UseOffscreenRendering;
  *css << this->Timeout;
  *css << this->UseIceT;
  *css << this->RenderModuleName;
  *css << this->OGVSupport;
  *css << this->AVISupport;
  *css << this->NumberOfProcesses;
  *css << this->GetNumberOfMachines();
  unsigned int idx;
  for (idx = 0; idx < this->GetNumberOfMachines(); idx++)
    {
    *css << this->GetEnvironment(idx);
    *css << this->GetLowerLeft(idx)[0] << this->GetLowerLeft(idx)[1]
         << this->GetLowerLeft(idx)[2];
    *css << this->GetLowerRight(idx)[0] << this->GetLowerRight(idx)[1]
         << this->GetLowerRight(idx)[2];
    *css << this->GetUpperRight(idx)[0] << this->GetUpperRight(idx)[1]
         << this->GetUpperRight(idx)[2];
    }
  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::CopyFromStream(const vtkClientServerStream* css)
{
  if(!css->GetArgument(0, 0, &this->RemoteRendering))
    {
    vtkErrorMacro("Error parsing RemoteRendering from message.");
    return;
    }
  if(   !css->GetArgument(0, 1, &this->TileDimensions[0])
     || !css->GetArgument(0, 2, &this->TileDimensions[1]) )
    {
    vtkErrorMacro("Error parsing TileDimensions from message.");
    return;
    }
  if(   !css->GetArgument(0, 3, &this->TileMullions[0])
     || !css->GetArgument(0, 4, &this->TileMullions[1]) )
    {
    vtkErrorMacro("Error parsing TileMullions from message.");
    return;
    }
  if(!css->GetArgument(0, 5, &this->UseOffscreenRendering))
    {
    vtkErrorMacro("Error parsing UseOffscreenRendering from message.");
    return;
    }
  if(!css->GetArgument(0, 6, &this->Timeout))
    {
    vtkErrorMacro("Error parsing Timeout from message.");
    return;
    }
  if (!css->GetArgument(0, 7, &this->UseIceT))
    {
    vtkErrorMacro("Error parsing IceT flag from message.");
    return;
    }
  const char *rmName;
  if (!css->GetArgument(0, 8, &rmName))
    {
    vtkErrorMacro("Error parsing render module name from message.");
    return;
    }
  this->SetRenderModuleName(rmName);
  if (!css->GetArgument(0, 9, &this->OGVSupport))
    {
    vtkErrorMacro("Error parsing OGVSupport flag from message.");
    return;
    }
  if (!css->GetArgument(0, 10, &this->AVISupport))
    {
    vtkErrorMacro("Error parsing AVISupport flag from message.");
    return;
    }
  int numProcs;
  if (!css->GetArgument(0, 11, &numProcs))
    {
    vtkErrorMacro("Error parsing number of processes from message.");
    return;
    }
  this->NumberOfProcesses = numProcs;

  unsigned int numMachines;
  if (!css->GetArgument(0, 12, &numMachines))
    {
    vtkErrorMacro("Error parsing number of machines from message.");
    return;
    }
  this->SetNumberOfMachines(numMachines);
  unsigned int idx;
  const char* env;
  for (idx = 0; idx < numMachines; idx++)
    {
    if (!css->GetArgument(0, 13 + idx*10, &env))
      {
      vtkErrorMacro("Error parsing display environment from message.");
      return;
      }
    this->MachinesInternals->MachineInformationVector[idx].Environment = env;
    if (!css->GetArgument(0, 14 + idx*10,
                          &this->MachinesInternals->MachineInformationVector[idx].LowerLeft[0]) ||
        !css->GetArgument(0, 15 + idx*10,
                          &this->MachinesInternals->MachineInformationVector[idx].LowerLeft[1]) ||
        !css->GetArgument(0, 16 + idx*10,
                          &this->MachinesInternals->MachineInformationVector[idx].LowerLeft[2]))
      {
      vtkErrorMacro("Error parsing lower left coordinate from message.");
      return;
      }
    if (!css->GetArgument(0, 17 + idx*10,
                          &this->MachinesInternals->MachineInformationVector[idx].LowerRight[0]) ||
        !css->GetArgument(0, 18 + idx*10,
                          &this->MachinesInternals->MachineInformationVector[idx].LowerRight[1]) ||
        !css->GetArgument(0, 19 + idx*10,
                          &this->MachinesInternals->MachineInformationVector[idx].LowerRight[2]))
      {
      vtkErrorMacro("Error parsing lower right coordinate from message.");
      return;
      }
    if (!css->GetArgument(0, 20 + idx*10,
                          &this->MachinesInternals->MachineInformationVector[idx].UpperRight[0]) ||
        !css->GetArgument(0, 21 + idx*10,
                          &this->MachinesInternals->MachineInformationVector[idx].UpperRight[1]) ||
        !css->GetArgument(0, 22 + idx*10,
                          &this->MachinesInternals->MachineInformationVector[idx].UpperRight[2]))
      {
      vtkErrorMacro("Error parsing upper left coordinate from message.");
      return;
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::SetNumberOfMachines(unsigned int num)
{
  delete this->MachinesInternals;
  this->MachinesInternals = new vtkPVServerOptionsInternals;
  unsigned int idx;
  vtkPVServerOptionsInternals::MachineInformation info;
  for (idx = 0; idx < num; idx++)
    {
    this->MachinesInternals->MachineInformationVector.push_back(info);
    }
}

//----------------------------------------------------------------------------
unsigned int vtkPVServerInformation::GetNumberOfMachines() const
{
  return static_cast<unsigned int>(
    this->MachinesInternals->MachineInformationVector.size());
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::SetEnvironment(unsigned int idx,
                                            const char* name)
{
  if (idx >= this->GetNumberOfMachines())
    {
    unsigned int i;
    vtkPVServerOptionsInternals::MachineInformation info;
    for (i = this->GetNumberOfMachines(); i <= idx; i++)
      {
      this->MachinesInternals->MachineInformationVector.push_back(info);
      }
    }

  this->MachinesInternals->MachineInformationVector[idx].Environment = name;
}

//----------------------------------------------------------------------------
const char* vtkPVServerInformation::GetEnvironment(unsigned int idx) const
{
  if (idx >= this->GetNumberOfMachines())
    {
    return NULL;
    }
  return this->MachinesInternals->MachineInformationVector[idx].Environment.c_str();
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::SetLowerLeft(unsigned int idx, double coord[3])
{
  if (idx >= this->GetNumberOfMachines())
    {
    unsigned int i;
    vtkPVServerOptionsInternals::MachineInformation info;
    for (i = this->GetNumberOfMachines(); i <= idx; i++)
      {
      this->MachinesInternals->MachineInformationVector.push_back(info);
      }
    }

  this->MachinesInternals->MachineInformationVector[idx].LowerLeft[0] = coord[0];
  this->MachinesInternals->MachineInformationVector[idx].LowerLeft[1] = coord[1];
  this->MachinesInternals->MachineInformationVector[idx].LowerLeft[2] = coord[2];
}

//----------------------------------------------------------------------------
double* vtkPVServerInformation::GetLowerLeft(unsigned int idx) const
{
  if (idx >= this->GetNumberOfMachines())
    {
    return NULL;
    }
  return this->MachinesInternals->MachineInformationVector[idx].LowerLeft;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::SetLowerRight(unsigned int idx, double coord[3])
{
  if (idx >= this->GetNumberOfMachines())
    {
    unsigned int i;
    vtkPVServerOptionsInternals::MachineInformation info;
    for (i = this->GetNumberOfMachines(); i <= idx; i++)
      {
      this->MachinesInternals->MachineInformationVector.push_back(info);
      }
    }

  this->MachinesInternals->MachineInformationVector[idx].LowerRight[0] = coord[0];
  this->MachinesInternals->MachineInformationVector[idx].LowerRight[1] = coord[1];
  this->MachinesInternals->MachineInformationVector[idx].LowerRight[2] = coord[2];
}

//----------------------------------------------------------------------------
double* vtkPVServerInformation::GetLowerRight(unsigned int idx) const
{
  if (idx >= this->GetNumberOfMachines())
    {
    return NULL;
    }
  return this->MachinesInternals->MachineInformationVector[idx].LowerRight;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::SetUpperRight(unsigned int idx, double coord[3])
{
  if (idx >= this->GetNumberOfMachines())
    {
    unsigned int i;
    vtkPVServerOptionsInternals::MachineInformation info;
    for (i = this->GetNumberOfMachines(); i <= idx; i++)
      {
      this->MachinesInternals->MachineInformationVector.push_back(info);
      }
    }

  this->MachinesInternals->MachineInformationVector[idx].UpperRight[0] = coord[0];
  this->MachinesInternals->MachineInformationVector[idx].UpperRight[1] = coord[1];
  this->MachinesInternals->MachineInformationVector[idx].UpperRight[2] = coord[2];
}

//----------------------------------------------------------------------------
double* vtkPVServerInformation::GetUpperRight(unsigned int idx) const
{
  if (idx >= this->GetNumberOfMachines())
    {
    return NULL;
    }
  return this->MachinesInternals->MachineInformationVector[idx].UpperRight;
}
