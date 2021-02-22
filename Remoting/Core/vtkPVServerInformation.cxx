/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerInformation.cxx

  Copyright (c) Kitware, Inc.
  Copyright (c) 2017, NVIDIA CORPORATION.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVServerInformation.h"

#include "vtkClientServerStream.h"
#include "vtkCompositeMultiProcessController.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVConfig.h"
#include "vtkPVConfig.h"
#include "vtkPVServerOptions.h"
#include "vtkPVServerOptionsInternals.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"
#if VTK_MODULE_ENABLE_ParaView_nvpipe
#include <nvpipe.h>
#endif

// ------------------------
// NOTE for OGVSupport
// ------------------------
// Ideally, we should include vtkIOMovieConfigure to determine if OGGTHEORA
// support has been enabled. However, this module cannot depend on vtkIOMovie
// (for Catalyst builds). Also, since vtkOggTheoraWriter is used in
// vtkPVServerManagerDefault module which adds a hard dependency on vtkIOMovie
// module, that can indeed check is OGGTHEORA support is available. So it's
// reasonably safe to assume OGGTHEORA is always enabled here.
// #include "vtkIOMovieConfigure.h"

#if VTK_MODULE_ENABLE_ParaView_nvpipe
//----------------------------------------------------------------------------
// NVPipe requires Kepler-class (or newer) NVIDIA hardware at runtime.  This
// verifies that such hardware is available.
static bool NVPipeAvailable()
{
  // Instantiate an encoder; this initializes CUDA and the NVEncode side of the
  // Video SDK, so if it succeeds we are good to go.
  nvpipe* dummy = nvpipe_create_encoder(NVPIPE_H264_NV, 1024);
  const bool success = dummy != nullptr;
  nvpipe_destroy(dummy);
  return success;
}
#endif

vtkStandardNewMacro(vtkPVServerInformation);

//----------------------------------------------------------------------------
vtkPVServerInformation::vtkPVServerInformation()
{
  this->MultiClientsEnable = 0;
  this->ClientId = 0;
  this->IdTypeSize = 0;
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  this->NumberOfProcesses = controller ? controller->GetNumberOfProcesses() : 1;
  this->MPIInitialized = controller ? controller->IsA("vtkMPIController") != 0 : false;
  this->RootOnly = 1;
  this->RemoteRendering = 1;
  this->TileDimensions[0] = this->TileDimensions[1] = 0;
  this->TileMullions[0] = this->TileMullions[1] = 0;
  this->Timeout = 0;
#if VTK_MODULE_ENABLE_ParaView_icet
  this->UseIceT = 1;
#else
  this->UseIceT = 0;
#endif

  this->AVISupport = 0;
#if defined(_WIN32)
  this->AVISupport = 1;
#else
#if VTK_MODULE_ENABLE_VTK_IOFFMPEG
  this->AVISupport = 1;
#endif
#endif

  this->NVPipeSupport = false;
#if VTK_MODULE_ENABLE_ParaView_nvpipe
  if (NVPipeAvailable())
  {
    this->NVPipeSupport = true;
  }
#endif

  // Refer to note at the top of this file abount OGVSupport.
  this->OGVSupport = 1;

  this->MachinesInternals = new vtkPVServerOptionsInternals;
}

//----------------------------------------------------------------------------
vtkPVServerInformation::~vtkPVServerInformation()
{
  delete this->MachinesInternals;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RemoteRendering: " << this->RemoteRendering << endl;
  os << indent << "TileDimensions: " << this->TileDimensions[0] << ", " << this->TileDimensions[1]
     << endl;
  os << indent << "TileMullions: " << this->TileMullions[0] << ", " << this->TileMullions[1]
     << endl;
  os << indent << "UseIceT: " << this->UseIceT << endl;
  os << indent << "OGVSupport: " << this->OGVSupport << endl;
  os << indent << "AVISupport: " << this->AVISupport << endl;
  os << indent << "Timeout: " << this->Timeout << endl;
  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
  os << indent << "MPIInitialized: " << this->MPIInitialized << endl;
  os << indent << "MultiClientsEnable: " << this->MultiClientsEnable << endl;
  os << indent << "ClientId: " << this->ClientId << endl;
  os << indent << "IdTypeSize: " << this->IdTypeSize << endl;
  os << indent << "NVPipeSupport: " << this->NVPipeSupport << endl;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::DeepCopy(vtkPVServerInformation* info)
{
  this->MultiClientsEnable = info->GetMultiClientsEnable();
  this->ClientId = info->GetClientId();
  this->IdTypeSize = info->GetIdTypeSize();
  this->RemoteRendering = info->GetRemoteRendering();
  info->GetTileDimensions(this->TileDimensions);
  info->GetTileMullions(this->TileMullions);
  this->UseIceT = info->GetUseIceT();
  this->Timeout = info->GetTimeout();
  this->SetNumberOfMachines(info->GetNumberOfMachines());
  unsigned int idx;
  for (idx = 0; idx < info->GetNumberOfMachines(); idx++)
  {
    this->SetEnvironment(idx, info->GetEnvironment(idx));
    this->SetGeometry(idx, info->GetGeometry(idx));
    this->SetFullScreen(idx, info->GetFullScreen(idx));
    this->SetShowBorders(idx, info->GetShowBorders(idx));
    this->SetStereoType(idx, info->GetStereoType(idx));
    this->SetLowerLeft(idx, info->GetLowerLeft(idx));
    this->SetLowerRight(idx, info->GetLowerRight(idx));
    this->SetUpperRight(idx, info->GetUpperRight(idx));
  }
  this->SetEyeSeparation(info->GetEyeSeparation());
  this->NumberOfProcesses = info->NumberOfProcesses;
  this->MPIInitialized = info->MPIInitialized;
  this->NVPipeSupport = info->NVPipeSupport;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::CopyFromObject(vtkObject* vtkNotUsed(obj))
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
  {
    vtkWarningMacro("ProcessModule is not available.");
    return;
  }

  // Options
  vtkPVOptions* options = pm->GetOptions();
  options->GetTileDimensions(this->TileDimensions);
  options->GetTileMullions(this->TileMullions);

  this->Timeout = options->GetTimeout();

  // Server options
  vtkPVServerOptions* serverOptions = vtkPVServerOptions::SafeDownCast(options);
  if (serverOptions)
  {
    this->MultiClientsEnable = serverOptions->GetMultiClientMode();
    this->SetNumberOfMachines(serverOptions->GetNumberOfMachines());
    unsigned int idx;
    for (idx = 0; idx < serverOptions->GetNumberOfMachines(); idx++)
    {
      this->SetEnvironment(idx, serverOptions->GetDisplayName(idx));
      this->SetGeometry(idx, serverOptions->GetGeometry(idx));
      this->SetFullScreen(idx, serverOptions->GetFullScreen(idx));
      this->SetShowBorders(idx, serverOptions->GetShowBorders(idx));
      this->SetStereoType(idx, serverOptions->GetStereoType(idx));
      this->SetLowerLeft(idx, serverOptions->GetLowerLeft(idx));
      this->SetLowerRight(idx, serverOptions->GetLowerRight(idx));
      this->SetUpperRight(idx, serverOptions->GetUpperRight(idx));
    }
    this->SetEyeSeparation(serverOptions->GetEyeSeparation());
  }

  vtkPVSession* session = vtkPVSession::SafeDownCast(pm->GetSession());
  vtkCompositeMultiProcessController* ctrl;
  if (session && (ctrl = vtkCompositeMultiProcessController::SafeDownCast(
                    session->GetController(vtkPVSession::CLIENT))))
  {
    this->ClientId = ctrl->GetActiveControllerID();
  }
  else
  {
    this->ClientId = 0;
  }

  this->IdTypeSize = static_cast<int>(8 * sizeof(vtkIdType));
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
    if (this->Timeout <= 0 ||
      (serverInfo->GetTimeout() > 0 && serverInfo->GetTimeout() < this->Timeout))
    {
      this->Timeout = serverInfo->GetTimeout();
    }

    if (!serverInfo->GetNVPipeSupport())
    {
      this->NVPipeSupport = false;
    }

    // IceT either is there or is not.
    this->UseIceT = serverInfo->GetUseIceT();
    this->SetNumberOfMachines(serverInfo->GetNumberOfMachines());
    unsigned int idx;
    for (idx = 0; idx < serverInfo->GetNumberOfMachines(); idx++)
    {
      this->SetEnvironment(idx, serverInfo->GetEnvironment(idx));
      this->SetGeometry(idx, serverInfo->GetGeometry(idx));
      this->SetFullScreen(idx, serverInfo->GetFullScreen(idx));
      this->SetShowBorders(idx, serverInfo->GetShowBorders(idx));
      this->SetStereoType(idx, serverInfo->GetStereoType(idx));
      this->SetLowerLeft(idx, serverInfo->GetLowerLeft(idx));
      this->SetLowerRight(idx, serverInfo->GetLowerRight(idx));
      this->SetUpperRight(idx, serverInfo->GetUpperRight(idx));
    }
    this->SetEyeSeparation(serverInfo->GetEyeSeparation());

    if (this->NumberOfProcesses < serverInfo->NumberOfProcesses)
    {
      this->NumberOfProcesses = serverInfo->NumberOfProcesses;
    }
    this->MPIInitialized = serverInfo->MPIInitialized;
    if (this->MultiClientsEnable < serverInfo->MultiClientsEnable)
    {
      this->MultiClientsEnable = serverInfo->MultiClientsEnable;
    }
    if (this->ClientId < serverInfo->ClientId)
    {
      this->ClientId = serverInfo->ClientId;
    }
    this->SetIdTypeSize(serverInfo->GetIdTypeSize());
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
  *css << 0; // we used to pass `>UseOffscreenRendering`.
  *css << this->Timeout;
  *css << this->UseIceT;
  *css << "<obsolete>"; // we used to pass RenderModuleName.
  *css << this->OGVSupport;
  *css << this->AVISupport;
  *css << this->NumberOfProcesses;
  *css << this->MPIInitialized;
  *css << this->GetNumberOfMachines();
  unsigned int idx;
  for (idx = 0; idx < this->GetNumberOfMachines(); idx++)
  {
    *css << this->GetEnvironment(idx);
    *css << this->GetGeometry(idx)[0] << this->GetGeometry(idx)[1] << this->GetGeometry(idx)[2]
         << this->GetGeometry(idx)[3];
    *css << this->GetFullScreen(idx);
    *css << this->GetShowBorders(idx);
    *css << this->GetStereoType(idx);
    *css << this->GetLowerLeft(idx)[0] << this->GetLowerLeft(idx)[1] << this->GetLowerLeft(idx)[2];
    *css << this->GetLowerRight(idx)[0] << this->GetLowerRight(idx)[1]
         << this->GetLowerRight(idx)[2];
    *css << this->GetUpperRight(idx)[0] << this->GetUpperRight(idx)[1]
         << this->GetUpperRight(idx)[2];
  }
  *css << this->GetEyeSeparation();
  *css << this->MultiClientsEnable;
  *css << this->ClientId;
  *css << this->IdTypeSize;
  *css << this->NVPipeSupport;
  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::CopyFromStream(const vtkClientServerStream* css)
{
  if (!css->GetArgument(0, 0, &this->RemoteRendering))
  {
    vtkErrorMacro("Error parsing RemoteRendering from message.");
    return;
  }
  if (!css->GetArgument(0, 1, &this->TileDimensions[0]) ||
    !css->GetArgument(0, 2, &this->TileDimensions[1]))
  {
    vtkErrorMacro("Error parsing TileDimensions from message.");
    return;
  }
  if (!css->GetArgument(0, 3, &this->TileMullions[0]) ||
    !css->GetArgument(0, 4, &this->TileMullions[1]))
  {
    vtkErrorMacro("Error parsing TileMullions from message.");
    return;
  }
  int obsolete_arg;
  if (!css->GetArgument(0, 5, &obsolete_arg))
  {
    vtkErrorMacro("Error parsing UseOffscreenRendering from message.");
    return;
  }
  if (!css->GetArgument(0, 6, &this->Timeout))
  {
    vtkErrorMacro("Error parsing Timeout from message.");
    return;
  }
  if (!css->GetArgument(0, 7, &this->UseIceT))
  {
    vtkErrorMacro("Error parsing IceT flag from message.");
    return;
  }
  // for client-server compatibility,
  const char* rmName;
  if (!css->GetArgument(0, 8, &rmName))
  {
    vtkErrorMacro("Error parsing render module name from message.");
    return;
  }
  // leaving this for client-server compatibility.
  // this->SetRenderModuleName(rmName);
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

  bool mpiInitialized;
  if (!css->GetArgument(0, 12, &mpiInitialized))
  {
    vtkErrorMacro("Error parsing mpi initialized from message.");
    return;
  }
  this->MPIInitialized = mpiInitialized;

  unsigned int numMachines;
  if (!css->GetArgument(0, 13, &numMachines))
  {
    vtkErrorMacro("Error parsing number of machines from message.");
    return;
  }
  this->SetNumberOfMachines(numMachines);
  unsigned int idx;
  const char* env;
  int machineOffset = 14;
  int valuesPerMachine = 17;
  for (idx = 0; idx < numMachines; idx++)
  {
    int machineIndex = machineOffset + idx * valuesPerMachine;
    int currentValueOffset = 0;
    if (!css->GetArgument(0, machineIndex + currentValueOffset++, &env))
    {
      vtkErrorMacro("Error parsing display environment from message for " << idx);
      return;
    }
    this->MachinesInternals->MachineInformationVector[idx].Environment = env;
    if (!css->GetArgument(0, machineIndex + currentValueOffset++,
          &this->MachinesInternals->MachineInformationVector[idx].Geometry[0]) ||
      !css->GetArgument(0, machineIndex + currentValueOffset++,
        &this->MachinesInternals->MachineInformationVector[idx].Geometry[1]) ||
      !css->GetArgument(0, machineIndex + currentValueOffset++,
        &this->MachinesInternals->MachineInformationVector[idx].Geometry[2]) ||
      !css->GetArgument(0, machineIndex + currentValueOffset++,
        &this->MachinesInternals->MachineInformationVector[idx].Geometry[3]))
    {
      vtkErrorMacro("Error parsing window geometry from message.");
      return;
    }
    if (!css->GetArgument(0, machineIndex + currentValueOffset++,
          &this->MachinesInternals->MachineInformationVector[idx].FullScreen))
    {
      vtkErrorMacro("Error parsing FullScreen flag from message.");
      return;
    }
    if (!css->GetArgument(0, machineIndex + currentValueOffset++,
          &this->MachinesInternals->MachineInformationVector[idx].ShowBorders))
    {
      vtkErrorMacro("Error parsing ShowBorders flag from message.");
      return;
    }
    if (!css->GetArgument(0, machineIndex + currentValueOffset++,
          &this->MachinesInternals->MachineInformationVector[idx].StereoType))
    {
      vtkErrorMacro("Error parsing StereoType flag from message.");
      return;
    }
    if (!css->GetArgument(0, machineIndex + currentValueOffset++,
          &this->MachinesInternals->MachineInformationVector[idx].LowerLeft[0]) ||
      !css->GetArgument(0, machineIndex + currentValueOffset++,
        &this->MachinesInternals->MachineInformationVector[idx].LowerLeft[1]) ||
      !css->GetArgument(0, machineIndex + currentValueOffset++,
        &this->MachinesInternals->MachineInformationVector[idx].LowerLeft[2]))
    {
      vtkErrorMacro("Error parsing lower left coordinate from message.");
      return;
    }
    if (!css->GetArgument(0, machineIndex + currentValueOffset++,
          &this->MachinesInternals->MachineInformationVector[idx].LowerRight[0]) ||
      !css->GetArgument(0, machineIndex + currentValueOffset++,
        &this->MachinesInternals->MachineInformationVector[idx].LowerRight[1]) ||
      !css->GetArgument(0, machineIndex + currentValueOffset++,
        &this->MachinesInternals->MachineInformationVector[idx].LowerRight[2]))
    {
      vtkErrorMacro("Error parsing lower right coordinate from message.");
      return;
    }
    if (!css->GetArgument(0, machineIndex + currentValueOffset++,
          &this->MachinesInternals->MachineInformationVector[idx].UpperRight[0]) ||
      !css->GetArgument(0, machineIndex + currentValueOffset++,
        &this->MachinesInternals->MachineInformationVector[idx].UpperRight[1]) ||
      !css->GetArgument(0, machineIndex + currentValueOffset++,
        &this->MachinesInternals->MachineInformationVector[idx].UpperRight[2]))
    {
      vtkErrorMacro("Error parsing upper left coordinate from message.");
      return;
    }
  }
  double eyeSeparation;
  int epilogueOffset = machineOffset + numMachines * valuesPerMachine;
  if (!css->GetArgument(0, epilogueOffset, &eyeSeparation))
  {
    vtkErrorMacro("Error parsing eye-separations from message.");
    return;
  }
  this->SetEyeSeparation(eyeSeparation);

  if (!css->GetArgument(0, epilogueOffset + 1, &this->MultiClientsEnable))
  {
    vtkErrorMacro("Error parsing MultiClientsEnable from message.");
    return;
  }
  if (!css->GetArgument(0, epilogueOffset + 2, &this->ClientId))
  {
    vtkErrorMacro("Error parsing ClientId from message.");
    return;
  }
  if (!css->GetArgument(0, epilogueOffset + 3, &this->IdTypeSize))
  {
    vtkErrorMacro("Error parsing IdTypeSize from message.");
    return;
  }
  if (!css->GetArgument(0, epilogueOffset + 4, &this->NVPipeSupport))
  {
    vtkErrorMacro("Error parsing NVPipeSupport from message.");
    return;
  }
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::SetEyeSeparation(double value)
{
  this->MachinesInternals->EyeSeparation = value;
}

//----------------------------------------------------------------------------
double vtkPVServerInformation::GetEyeSeparation() const
{
  return static_cast<double>(this->MachinesInternals->EyeSeparation);
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
  return static_cast<unsigned int>(this->MachinesInternals->MachineInformationVector.size());
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::SetEnvironment(unsigned int idx, const char* name)
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
    return nullptr;
  }
  return this->MachinesInternals->MachineInformationVector[idx].Environment.c_str();
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::SetGeometry(unsigned int idx, int geo[])
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

  for (int i = 0; i < 4; ++i)
  {
    this->MachinesInternals->MachineInformationVector[idx].Geometry[i] = geo[i];
  }
}

//----------------------------------------------------------------------------
int* vtkPVServerInformation::GetGeometry(unsigned int idx) const
{
  if (idx >= this->GetNumberOfMachines())
  {
    return nullptr;
  }
  return this->MachinesInternals->MachineInformationVector[idx].Geometry;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::SetFullScreen(unsigned int idx, bool fullscreen)
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

  this->MachinesInternals->MachineInformationVector[idx].FullScreen = fullscreen;
}

//----------------------------------------------------------------------------
bool vtkPVServerInformation::GetFullScreen(unsigned int idx) const
{
  if (idx >= this->GetNumberOfMachines())
  {
    return false;
  }

  return this->MachinesInternals->MachineInformationVector[idx].FullScreen;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::SetShowBorders(unsigned int idx, bool borders)
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

  this->MachinesInternals->MachineInformationVector[idx].ShowBorders = borders;
}

//----------------------------------------------------------------------------
bool vtkPVServerInformation::GetShowBorders(unsigned int idx) const
{
  if (idx >= this->GetNumberOfMachines())
  {
    return false;
  }

  return this->MachinesInternals->MachineInformationVector[idx].ShowBorders;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::SetStereoType(unsigned int idx, int type)
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

  this->MachinesInternals->MachineInformationVector[idx].StereoType = type;
}

//----------------------------------------------------------------------------
int vtkPVServerInformation::GetStereoType(unsigned int idx) const
{
  if (idx >= this->GetNumberOfMachines())
  {
    return -1;
  }

  return this->MachinesInternals->MachineInformationVector[idx].StereoType;
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
    return nullptr;
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
    return nullptr;
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
    return nullptr;
  }
  return this->MachinesInternals->MachineInformationVector[idx].UpperRight;
}

//----------------------------------------------------------------------------
bool vtkPVServerInformation::IsMPIInitialized() const
{
  return this->MPIInitialized;
}
