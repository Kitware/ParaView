/*=========================================================================

  Module:    vtkPVOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVOptions.h"

#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVOptionsXMLParser.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConfiguration.h"
#include "vtkRemotingCoreConfiguration.h"

#include <vtksys/CommandLineArguments.hxx>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVOptions);

//----------------------------------------------------------------------------
vtkPVOptions::vtkPVOptions()
{
  this->SetProcessType(ALLPROCESS);
  if (this->XMLParser)
  {
    this->XMLParser->Delete();
    this->XMLParser = nullptr;
  }
  this->XMLParser = vtkPVOptionsXMLParser::New();
  this->XMLParser->SetPVOptions(this);
}

//----------------------------------------------------------------------------
vtkPVOptions::~vtkPVOptions() = default;

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
const char* vtkPVOptions::GetHostName()
{
  VTK_LEGACY_REPLACED_BODY(
    vtkPVOptions::GetHostName, "ParaView 5.10", vtkRemotingCoreConfiguration::GetHostName);
  const auto& hostname = vtkRemotingCoreConfiguration::GetInstance()->GetHostName();
  return hostname.empty() ? nullptr : hostname.c_str();
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int vtkPVOptions::GetConnectID()
{
  VTK_LEGACY_REPLACED_BODY(
    vtkPVOptions::GetConnectID(), "ParaView 5.10", vtkRemotingCoreConfiguration::GetConnectID);
  return vtkRemotingCoreConfiguration::GetInstance()->GetConnectID();
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
void vtkPVOptions::SetConnectID(int val)
{
  VTK_LEGACY_REPLACED_BODY(
    vtkPVOptions::SetConnectID(), "ParaView 5.10", vtkRemotingCoreConfiguration::SetConnectID);
  vtkRemotingCoreConfiguration::GetInstance()->SetConnectID(val);
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int vtkPVOptions::GetTellVersion()
{
  VTK_LEGACY_REPLACED_BODY(
    vtkPVOptions::GetTellVersion, "ParaView 5.10", vtkRemotingCoreConfiguration::GetTellVersion);
  return vtkRemotingCoreConfiguration::GetInstance()->GetTellVersion() ? 1 : 0;
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int vtkPVOptions::GetSymmetricMPIMode()
{
  VTK_LEGACY_REPLACED_BODY(vtkPVOptions::GetSymmetricMPIMode, "ParaView 5.10",
    vtkProcessModuleConfiguration::GetSymmetricMPIMode);
  return vtkProcessModuleConfiguration::GetInstance()->GetSymmetricMPIMode() ? 1 : 0;
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int vtkPVOptions::GetEnableStackTrace()
{
  VTK_LEGACY_REPLACED_BODY(vtkPVOptions::GetEnableStackTrace, "ParaView 5.10",
    vtkProcessModuleConfiguration::GetEnableStackTrace);
  return vtkProcessModuleConfiguration::GetInstance()->GetEnableStackTrace() ? 1 : 0;
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int vtkPVOptions::GetTimeout()
{
  VTK_LEGACY_REPLACED_BODY(
    vtkPVOptions::GetTimeout, "ParaView 5.10", vtkRemotingCoreConfiguration::GetTimeout);
  return vtkRemotingCoreConfiguration::GetInstance()->GetTimeout();
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
const char* vtkPVOptions::GetLogFileName()
{
  VTK_LEGACY_REPLACED_BODY(
    vtkPVOptions::GetLogFileName, "ParaView 5.10", vtkProcessModuleConfiguration::GetCSLogFileName);
  const auto& fname = vtkProcessModuleConfiguration::GetInstance()->GetCSLogFileName();
  return fname.empty() ? nullptr : fname.c_str();
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int vtkPVOptions::GetReverseConnection()
{
  VTK_LEGACY_REPLACED_BODY(vtkPVOptions::GetReverseConnection, "ParaView 5.10",
    vtkRemotingCoreConfiguration::GetReverseConnection);
  return vtkRemotingCoreConfiguration::GetInstance()->GetReverseConnection() ? 1 : 0;
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
const char* vtkPVOptions::GetServerURL()
{
  VTK_LEGACY_REPLACED_BODY(
    vtkPVOptions::GetServerURL, "ParaView 5.10", vtkRemotingCoreConfiguration::GetServerURL);
  const auto& url = vtkRemotingCoreConfiguration::GetInstance()->GetServerURL();
  return url.empty() ? nullptr : url.c_str();
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
const char* vtkPVOptions::GetServersFileName()
{
  VTK_LEGACY_REPLACED_BODY(vtkPVOptions::GetServersFileName, "ParaView 5.10",
    vtkRemotingCoreConfiguration::GetServerConfigurationsFiles);
  const auto& fnames = vtkRemotingCoreConfiguration::GetInstance()->GetServerConfigurationsFiles();
  return fnames.empty() ? nullptr : fnames.front().c_str();
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
const char* vtkPVOptions::GetStereoType()
{
  VTK_LEGACY_REPLACED_BODY(
    vtkPVOptions::GetStereoType, "ParaView 5.10", vtkRemotingCoreConfiguration::GetStereoType);
  return vtkRemotingCoreConfiguration::GetInstance()->GetStereoTypeAsString();
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int vtkPVOptions::GetUseStereoRendering()
{
  VTK_LEGACY_REPLACED_BODY(vtkPVOptions::GetUseStereoRendering, "ParaView 5.10",
    vtkRemotingCoreConfiguration::GetUseStereoRendering);
  return vtkRemotingCoreConfiguration::GetInstance()->GetUseStereoRendering() ? 1 : 0;
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
const int* vtkPVOptions::GetTileDimensions()
{
  VTK_LEGACY_REPLACED_BODY(vtkPVOptions::GetTileDimensions, "ParaView 5.10",
    vtkRemotingCoreConfiguration::GetTileDimensions);
  return vtkRemotingCoreConfiguration::GetInstance()->GetTileDimensions();
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
void vtkPVOptions::GetTileDimensions(int dims[2])
{
  VTK_LEGACY_REPLACED_BODY(vtkPVOptions::GetTileDimensions, "ParaView 5.10",
    vtkRemotingCoreConfiguration::GetTileDimensions);
  vtkRemotingCoreConfiguration::GetInstance()->GetTileDimensions(dims);
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
const int* vtkPVOptions::GetTileMullions()
{
  VTK_LEGACY_REPLACED_BODY(
    vtkPVOptions::GetTileMullions, "ParaView 5.10", vtkRemotingCoreConfiguration::GetTileMullions);
  return vtkRemotingCoreConfiguration::GetInstance()->GetTileMullions();
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
void vtkPVOptions::GetTileMullions(int dims[2])
{
  VTK_LEGACY_REPLACED_BODY(
    vtkPVOptions::GetTileMullions, "ParaView 5.10", vtkRemotingCoreConfiguration::GetTileMullions);
  vtkRemotingCoreConfiguration::GetInstance()->GetTileMullions(dims);
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
bool vtkPVOptions::GetIsInTileDisplay() const
{
  VTK_LEGACY_REPLACED_BODY(vtkPVOptions::GetIsInTileDisplay, "ParaView 5.10",
    vtkRemotingCoreConfiguration::GetIsInTileDisplay);
  return vtkRemotingCoreConfiguration::GetInstance()->GetIsInTileDisplay();
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
bool vtkPVOptions::GetIsInCave() const
{
  VTK_LEGACY_REPLACED_BODY(
    vtkPVOptions::GetIsInCave, "ParaView 5.10", vtkRemotingCoreConfiguration::GetIsInCave);
  return vtkRemotingCoreConfiguration::GetInstance()->GetIsInCave();
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int vtkPVOptions::GetForceOffscreenRendering()
{
  VTK_LEGACY_REPLACED_BODY(vtkPVOptions::GetForceOffscreenRendering, "ParaView 5.10",
    vtkRemotingCoreConfiguration::GetForceOffscreenRendering);
  return vtkRemotingCoreConfiguration::GetInstance()->GetForceOffscreenRendering() ? 1 : 0;
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int vtkPVOptions::GetForceOnscreenRendering()
{
  VTK_LEGACY_REPLACED_BODY(vtkPVOptions::GetForceOnscreenRendering, "ParaView 5.10",
    vtkRemotingCoreConfiguration::GetForceOnscreenRendering);
  return vtkRemotingCoreConfiguration::GetInstance()->GetForceOnscreenRendering() ? 1 : 0;
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int vtkPVOptions::GetDisableXDisplayTests()
{
  VTK_LEGACY_REPLACED_BODY(vtkPVOptions::GetDisableXDisplayTests, "ParaView 5.10",
    vtkRemotingCoreConfiguration::GetDisableXDisplayTests);
  return vtkRemotingCoreConfiguration::GetInstance()->GetDisableXDisplayTests() ? 1 : 0;
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int vtkPVOptions::GetEGLDeviceIndex()
{
  VTK_LEGACY_REPLACED_BODY(vtkPVOptions::GetEGLDeviceIndex, "ParaView 5.10",
    vtkRemotingCoreConfiguration::GetEGLDeviceIndex);
  return vtkRemotingCoreConfiguration::GetInstance()->GetEGLDeviceIndex();
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int vtkPVOptions::GetMultiClientMode()
{
  VTK_LEGACY_REPLACED_BODY(vtkPVOptions::GetMultiClientMode, "ParaView 5.10",
    vtkRemotingCoreConfiguration::GetMultiClientMode);
  return vtkRemotingCoreConfiguration::GetInstance()->GetMultiClientMode() ? 1 : 0;
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int vtkPVOptions::IsMultiClientModeDebug()
{
  VTK_LEGACY_BODY(vtkPVOptions::IsMultiClientModeDebug, "ParaView 5.10");
  return false;
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int vtkPVOptions::GetDisableFurtherConnections()
{
  VTK_LEGACY_REPLACED_BODY(vtkPVOptions::GetDisableFurtherConnections, "ParaView 5.10",
    vtkRemotingCoreConfiguration::GetDisableFurtherConnections);
  return vtkRemotingCoreConfiguration::GetInstance()->GetDisableFurtherConnections() ? 1 : 0;
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int vtkPVOptions::GetMultiServerMode()
{
  VTK_LEGACY_REPLACED_BODY(vtkPVOptions::GetMultiServerMode, "ParaView 5.10",
    vtkRemotingCoreConfiguration::GetMultiServerMode);
  return vtkRemotingCoreConfiguration::GetInstance()->GetMultiServerMode() ? 1 : 0;
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
const char* vtkPVOptions::GetParaViewDataName()
{
  VTK_LEGACY_BODY(vtkPVOptions::GetParaViewDataName, "ParaView 5.10");
  return nullptr;
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
const char* vtkPVOptions::GetTestPlugins()
{
  VTK_LEGACY_BODY(vtkPVOptions::GetTestPlugins, "ParaView 5.10");
  return nullptr;
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
const char* vtkPVOptions::GetTestPluginPaths()
{
  VTK_LEGACY_BODY(vtkPVOptions::GetTestPluginPaths, "ParaView 5.10");
  return nullptr;
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int vtkPVOptions::GetCatalystLivePort()
{
  VTK_LEGACY_BODY(vtkPVOptions::GetCatalystLivePort, "ParaView 5.10");
  return -1;
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int vtkPVOptions::GetDisableRegistry()
{
  VTK_LEGACY_REPLACED_BODY(vtkPVOptions::GetDisableRegistry, "ParaView 5.10",
    vtkRemotingCoreConfiguration::GetDisableRegistry);
  return vtkRemotingCoreConfiguration::GetInstance()->GetDisableRegistry() ? 1 : 0;
}
#endif

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int vtkPVOptions::GetPrintMonitors()
{
  VTK_LEGACY_REPLACED_BODY(vtkPVOptions::GetPrintMonitors, "ParaView 5.10",
    vtkRemotingCoreConfiguration::GetPrintMonitors);
  return vtkRemotingCoreConfiguration::GetInstance()->GetPrintMonitors() ? 1 : 0;
}
#endif

//----------------------------------------------------------------------------
void vtkPVOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
