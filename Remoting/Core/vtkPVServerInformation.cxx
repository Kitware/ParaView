// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) 2017, NVIDIA CORPORATION
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVServerInformation.h"

#include "vtkClientServerStream.h"
#include "vtkCompositeMultiProcessController.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"
#include "vtkRemotingCoreConfiguration.h"
#include "vtkSMPTools.h"
#if VTK_MODULE_ENABLE_ParaView_nvpipe
#include <nvpipe.h>
#endif

#include <algorithm>

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
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  this->NumberOfProcesses = controller ? controller->GetNumberOfProcesses() : 1;
  this->MPIInitialized = controller ? controller->IsA("vtkMPIController") != 0 : false;
  this->RootOnly = 1;

#if VTK_MODULE_ENABLE_ParaView_icet
  this->UseIceT = 1;
#else
  this->UseIceT = 0;
#endif

#if defined(_WIN32)
  this->AVISupport = 1;
#else
#if VTK_MODULE_ENABLE_VTK_IOFFMPEG
  this->AVISupport = 1;
#endif
#endif

#if VTK_MODULE_ENABLE_ParaView_nvpipe
  if (NVPipeAvailable())
  {
    this->NVPipeSupport = true;
  }
#endif

  this->SMPBackendName = vtkSMPTools::GetBackend() ? vtkSMPTools::GetBackend() : "";
  this->SMPMaxNumberOfThreads = vtkSMPTools::GetEstimatedNumberOfThreads();

#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmFilters && VTK_ENABLE_VISKORES_OVERRIDES
  this->AcceleratedFiltersOverrideAvailable = true;
#else
  this->AcceleratedFiltersOverrideAvailable = false;
#endif
}

//----------------------------------------------------------------------------
vtkPVServerInformation::~vtkPVServerInformation() = default;

//----------------------------------------------------------------------------
void vtkPVServerInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RemoteRendering: " << this->RemoteRendering << endl;
  os << indent << "UseIceT: " << this->UseIceT << endl;
  os << indent << "OGVSupport: " << this->OGVSupport << endl;
  os << indent << "AVISupport: " << this->AVISupport << endl;
  os << indent << "Timeout: " << this->Timeout << endl;
  os << indent << "TimeoutCommand: " << this->TimeoutCommand << endl;
  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
  os << indent << "MPIInitialized: " << this->MPIInitialized << endl;
  os << indent << "MultiClientsEnable: " << this->MultiClientsEnable << endl;
  os << indent << "ClientId: " << this->ClientId << endl;
  os << indent << "IdTypeSize: " << this->IdTypeSize << endl;
  os << indent << "NVPipeSupport: " << this->NVPipeSupport << endl;
  os << indent << "IsInTileDisplay: " << this->IsInTileDisplay << endl;
  os << indent << "IsInCave: " << this->IsInCave << endl;
  os << indent << "TileDimensions: " << this->TileDimensions[0] << ", " << this->TileDimensions[1]
     << endl;
  os << indent << "SMPBackendName: " << this->SMPBackendName << endl;
  os << indent << "SMPMaxNumberOfThreads: " << this->SMPMaxNumberOfThreads << endl;
  os << indent
     << "AcceleratedFiltersOverrideAvailable: " << this->AcceleratedFiltersOverrideAvailable
     << endl;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::DeepCopy(vtkPVServerInformation* info)
{
  this->MultiClientsEnable = info->GetMultiClientsEnable();
  this->ClientId = info->GetClientId();
  this->IdTypeSize = info->GetIdTypeSize();
  this->RemoteRendering = info->GetRemoteRendering();
  this->UseIceT = info->GetUseIceT();
  this->Timeout = info->GetTimeout();
  this->TimeoutCommand = info->GetTimeoutCommand();
  this->TimeoutCommandInterval = info->GetTimeoutCommandInterval();
  this->NumberOfProcesses = info->NumberOfProcesses;
  this->MPIInitialized = info->MPIInitialized;
  this->NVPipeSupport = info->NVPipeSupport;
  this->IsInTileDisplay = info->GetIsInTileDisplay();
  this->IsInCave = info->GetIsInCave();
  info->GetTileDimensions(this->TileDimensions);
  this->SMPBackendName = info->GetSMPBackendName();
  this->SMPMaxNumberOfThreads = info->GetSMPMaxNumberOfThreads();
  this->AcceleratedFiltersOverrideAvailable = info->GetAcceleratedFiltersOverrideAvailable();
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

  auto config = vtkRemotingCoreConfiguration::GetInstance();
  this->Timeout = config->GetTimeout();
  this->TimeoutCommand = config->GetTimeoutCommand();
  this->TimeoutCommandInterval = config->GetTimeoutCommandInterval();
  this->IsInCave = config->GetIsInCave();
  this->IsInTileDisplay = config->GetIsInTileDisplay();
  this->MultiClientsEnable = config->GetMultiClientMode();
  config->GetTileDimensions(this->TileDimensions);

  vtkPVSession* session = vtkPVSession::SafeDownCast(pm->GetSession());
  vtkCompositeMultiProcessController* ctrl;
  if (session &&
    (ctrl = vtkCompositeMultiProcessController::SafeDownCast(
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
    if (this->Timeout <= 0 ||
      (serverInfo->GetTimeout() > 0 && serverInfo->GetTimeout() < this->Timeout))
    {
      this->Timeout = serverInfo->GetTimeout();
    }

    this->TimeoutCommand = serverInfo->GetTimeoutCommand();
    this->TimeoutCommandInterval = serverInfo->GetTimeoutCommandInterval();

    this->IsInTileDisplay |= serverInfo->GetIsInTileDisplay();
    this->IsInTileDisplay |= serverInfo->GetIsInCave();

    if (serverInfo->GetTileDimensions()[0])
    {
      serverInfo->GetTileDimensions(this->TileDimensions);
    }

    if (!serverInfo->GetNVPipeSupport())
    {
      this->NVPipeSupport = false;
    }

    // IceT either is there or is not.
    this->UseIceT = serverInfo->GetUseIceT();
    this->NumberOfProcesses = std::max(this->NumberOfProcesses, serverInfo->NumberOfProcesses);
    this->MPIInitialized = serverInfo->MPIInitialized;
    this->MultiClientsEnable = std::max(this->MultiClientsEnable, serverInfo->MultiClientsEnable);
    this->ClientId = std::max(this->ClientId, serverInfo->ClientId);
    this->SMPBackendName = serverInfo->GetSMPBackendName();
    this->SMPMaxNumberOfThreads = serverInfo->GetSMPMaxNumberOfThreads();
    this->SetIdTypeSize(serverInfo->GetIdTypeSize());
    this->AcceleratedFiltersOverrideAvailable =
      serverInfo->GetAcceleratedFiltersOverrideAvailable();
  }
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;
  *css << this->RemoteRendering;
  *css << this->Timeout;
  *css << this->TimeoutCommand;
  *css << this->TimeoutCommandInterval;
  *css << this->UseIceT;
  *css << this->OGVSupport;
  *css << this->AVISupport;
  *css << this->NumberOfProcesses;
  *css << this->MPIInitialized;
  *css << this->MultiClientsEnable;
  *css << this->ClientId;
  *css << this->IdTypeSize;
  *css << this->NVPipeSupport;
  *css << this->IsInTileDisplay;
  *css << this->IsInCave;
  *css << this->TileDimensions[0] << this->TileDimensions[1];
  *css << this->SMPBackendName;
  *css << this->SMPMaxNumberOfThreads;
  *css << this->AcceleratedFiltersOverrideAvailable;
  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::CopyFromStream(const vtkClientServerStream* css)
{
  int idx = 0;
  if (!css->GetArgument(0, idx++, &this->RemoteRendering))
  {
    vtkErrorMacro("Error parsing RemoteRendering from message.");
    return;
  }

  if (!css->GetArgument(0, idx++, &this->Timeout))
  {
    vtkErrorMacro("Error parsing Timeout from message.");
    return;
  }
  if (!css->GetArgument(0, idx++, &this->TimeoutCommand))
  {
    vtkErrorMacro("Error parsing TimeoutCommand from message.");
    return;
  }
  if (!css->GetArgument(0, idx++, &this->TimeoutCommandInterval))
  {
    vtkErrorMacro("Error parsing TimeoutCommandInterval from message.");
    return;
  }
  if (!css->GetArgument(0, idx++, &this->UseIceT))
  {
    vtkErrorMacro("Error parsing IceT flag from message.");
    return;
  }
  if (!css->GetArgument(0, idx++, &this->OGVSupport))
  {
    vtkErrorMacro("Error parsing OGVSupport flag from message.");
    return;
  }
  if (!css->GetArgument(0, idx++, &this->AVISupport))
  {
    vtkErrorMacro("Error parsing AVISupport flag from message.");
    return;
  }
  int numProcs;
  if (!css->GetArgument(0, idx++, &numProcs))
  {
    vtkErrorMacro("Error parsing number of processes from message.");
    return;
  }
  this->NumberOfProcesses = numProcs;

  bool mpiInitialized;
  if (!css->GetArgument(0, idx++, &mpiInitialized))
  {
    vtkErrorMacro("Error parsing mpi initialized from message.");
    return;
  }
  this->MPIInitialized = mpiInitialized;

  if (!css->GetArgument(0, idx++, &this->MultiClientsEnable))
  {
    vtkErrorMacro("Error parsing MultiClientsEnable from message.");
    return;
  }
  if (!css->GetArgument(0, idx++, &this->ClientId))
  {
    vtkErrorMacro("Error parsing ClientId from message.");
    return;
  }
  if (!css->GetArgument(0, idx++, &this->IdTypeSize))
  {
    vtkErrorMacro("Error parsing IdTypeSize from message.");
    return;
  }
  if (!css->GetArgument(0, idx++, &this->NVPipeSupport))
  {
    vtkErrorMacro("Error parsing NVPipeSupport from message.");
    return;
  }

  if (!css->GetArgument(0, idx++, &this->IsInTileDisplay))
  {
    vtkErrorMacro("Error parsing IsInTileDisplay from message.");
    return;
  }
  if (!css->GetArgument(0, idx++, &this->IsInCave))
  {
    vtkErrorMacro("Error parsing IsInCave from message.");
    return;
  }

  if (!css->GetArgument(0, idx++, &this->TileDimensions[0]) ||
    !css->GetArgument(0, idx++, &this->TileDimensions[1]))
  {
    vtkErrorMacro("Error parsing TileDimensions from message.");
    return;
  }

  if (!css->GetArgument(0, idx++, &this->SMPBackendName))
  {
    vtkErrorMacro("Error parsing SMPBackendName from message.");
    return;
  }
  if (!css->GetArgument(0, idx++, &this->SMPMaxNumberOfThreads))
  {
    vtkErrorMacro("Error parsing SMPMaxNumberOfThreads from message.");
    return;
  }
  if (!css->GetArgument(0, idx++, &this->AcceleratedFiltersOverrideAvailable))
  {
    vtkErrorMacro("Error parsing AcceleratedFiltersOverrideAvailable from message.");
    return;
  }
}

//----------------------------------------------------------------------------
bool vtkPVServerInformation::IsMPIInitialized() const
{
  return this->MPIInitialized;
}
