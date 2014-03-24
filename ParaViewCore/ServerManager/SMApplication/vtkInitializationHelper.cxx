/*=========================================================================

Program:   ParaView
Module:    vtkInitializationHelper.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInitializationHelper.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerInterpreterInitializer.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkOutputWindow.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPVInitializer.h"
#include "vtkPVOptions.h"
#include "vtkPVPluginLoader.h"
#include "vtkPVSession.h"
#include "vtkSMSettings.h"
#include "vtkSmartPointer.h"
#include "vtkSMMessage.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"

#include <string>
#include <vtksys/ios/sstream>

// Windows-only helper functionality:
#ifdef _WIN32
# include <Windows.h>
#endif

namespace {

#ifdef _WIN32
BOOL CALLBACK listMonitorsCallback(HMONITOR hMonitor, HDC /*hdcMonitor*/,
                                   LPRECT /*lprcMonitor*/, LPARAM dwData)
{
  std::ostringstream *str = reinterpret_cast<std::ostringstream*>(dwData);

  MONITORINFOEX monitorInfo;
  monitorInfo.cbSize = sizeof(monitorInfo);

  if (GetMonitorInfo(hMonitor, &monitorInfo))
    {
      LPRECT rect = &monitorInfo.rcMonitor;
      *str << "Device: \"" << monitorInfo.szDevice << "\" "
           << "Geometry: "
           << std::noshowpos
           << rect->right - rect->left << "x"
           << rect->bottom - rect->top
           << std::showpos
           << rect->left << rect->top << " "
           << ((monitorInfo.dwFlags & MONITORINFOF_PRIMARY)
               ? "(primary)" : "")
           << std::endl;
    }
  return true;
}
#endif // _WIN32

std::string ListAttachedMonitors()
{
#ifndef _WIN32
  return std::string("Monitor detection only implemented for MS Windows.");
#else // _WIN32
  std::ostringstream str;
  EnumDisplayMonitors(NULL, NULL, listMonitorsCallback,
                      reinterpret_cast<LPARAM>(&str));
  return str.str();
#endif // _WIN32
}

} // end anon namespace

//----------------------------------------------------------------------------
void vtkInitializationHelper::Initialize(const char* executable, int type)
{
  vtkInitializationHelper::Initialize(executable, type, NULL);
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::Initialize(const char* executable,
  int type, vtkPVOptions* options)
{
  if (!executable)
    {
    vtkGenericWarningMacro("Executable name has to be defined.");
    return;
    }

  // Pass the program name to make option parser happier
  char* argv = new char[strlen(executable)+1];
  strcpy(argv, executable);

  vtkSmartPointer<vtkPVOptions> newoptions = options;
  if (!options)
    {
    newoptions = vtkSmartPointer<vtkPVOptions>::New();
    }
  vtkInitializationHelper::Initialize(1, &argv, type, newoptions);
  delete[] argv;
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::Initialize(int argc, char**argv,
  int type, vtkPVOptions* options)
{
  if (vtkProcessModule::GetProcessModule())
    {
    vtkGenericWarningMacro("Process already initialize. Skipping.");
    return;
    }

  if (!options)
    {
    vtkGenericWarningMacro("vtkPVOptions must be specified.");
    return;
    }

  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  vtkProcessModule::Initialize(
    static_cast<vtkProcessModule::ProcessTypes>(type), argc, argv);

  vtksys_ios::ostringstream sscerr;
  if (argv && !options->Parse(argc, argv) )
    {
    if ( options->GetUnknownArgument() )
      {
      sscerr << "Got unknown argument: " << options->GetUnknownArgument() << endl;
      }
    if ( options->GetErrorMessage() )
      {
      sscerr << "Error: " << options->GetErrorMessage() << endl;
      }
    options->SetHelpSelected(1);
    }
  if (options->GetHelpSelected())
    {
    sscerr << options->GetHelp() << endl;
    vtkOutputWindow::GetInstance()->DisplayText( sscerr.str().c_str() );
    // TODO: indicate to the caller that application must quit.
    }

  if (options->GetTellVersion() )
    {
    vtksys_ios::ostringstream str;
    str << "paraview version " << PARAVIEW_VERSION_FULL << "\n";
    vtkOutputWindow::GetInstance()->DisplayText(str.str().c_str());
    // TODO: indicate to the caller that application must quit.
    }

  if (options->GetPrintMonitors())
    {
      std::string monitors = ListAttachedMonitors();
      vtkOutputWindow::GetInstance()->DisplayText(monitors.c_str());
      // TODO: indicate to the caller that application must quit.
    }

  vtkProcessModule::GetProcessModule()->SetOptions(options);

  // this has to happen after process module is initialized and options have
  // been set.
  PARAVIEW_INITIALIZE();

  // Set multi-server flag to vtkProcessModule
  vtkProcessModule::GetProcessModule()->SetMultipleSessionsSupport(
        options->GetMultiServerMode() != 0);

  // Make sure the ProxyManager get created...
  vtkSMProxyManager::GetProxyManager();

  // Now load any plugins located in the PV_PLUGIN_PATH environment variable.
  // These are always loaded (not merely located).
  vtkNew<vtkPVPluginLoader> loader;
  loader->LoadPluginsFromPluginSearchPath();

  // Load settings files. Only load settings on client or root data node
  vtkSMSettings * settings = vtkSMSettings::GetInstance();
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  if (pm->GetProcessType() == vtkProcessModule::PROCESS_CLIENT ||
      (pm->GetProcessType() == vtkProcessModule::PROCESS_BATCH &&
       pm->GetPartitionId() == 0))
    {
    settings->LoadSiteSettings();
    settings->LoadUserSettings();
    }
  if (pm->GetProcessType() == vtkProcessModule::PROCESS_BATCH &&
      pm->GetSymmetricMPIMode())
    {
    // Broadcast settings to satellite nodes
    vtkMultiProcessController * controller = pm->GetGlobalController();
    unsigned int stringSize;
    if (controller->GetLocalProcessId() == 0)
      {
      std::string siteSettingsString = settings->GetSiteSettingsAsString().c_str();
      stringSize = static_cast<unsigned int>(siteSettingsString.size())+1;
      controller->Broadcast(&stringSize, 1, 0);
      if (stringSize > 0)
        {
        controller->Broadcast(const_cast<char*>(siteSettingsString.c_str()), stringSize, 0);
        }

      std::string userSettingsString = settings->GetUserSettingsAsString();
      stringSize = static_cast<unsigned int>(userSettingsString.size())+1;
      controller->Broadcast(&stringSize, 1, 0);
      if (stringSize > 0)
        {
        controller->Broadcast(const_cast<char*>(userSettingsString.c_str()), stringSize, 0);
        }
      }
    else // Satellites
      {
      controller->Broadcast(&stringSize, 1, 0);
      if (stringSize > 0)
        {
        char* siteSettingsString = new char[stringSize];
        controller->Broadcast(siteSettingsString, stringSize, 0);
        settings->SetSiteSettingsFromString(siteSettingsString);
        delete[] siteSettingsString;
        }
      else
        {
        settings->SetSiteSettingsFromString(NULL);
        }
      controller->Broadcast(&stringSize, 1, 0);
      if (stringSize > 0)
        {
        char* userSettingsString = new char[stringSize];
        controller->Broadcast(userSettingsString, stringSize, 0);
        settings->SetUserSettingsFromString(userSettingsString);
        delete[] userSettingsString;
        }
      else
        {
        settings->SetUserSettingsFromString(NULL);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::StandaloneInitialize()
{
  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::Finalize()
{
  vtkSMProxyManager::Finalize();
  vtkProcessModule::Finalize();

  // Optional:  Delete all global objects allocated by libprotobuf.
  google::protobuf::ShutdownProtobufLibrary();
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::StandaloneFinalize()
{
  // Optional:  Delete all global objects allocated by libprotobuf.
  google::protobuf::ShutdownProtobufLibrary();
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
