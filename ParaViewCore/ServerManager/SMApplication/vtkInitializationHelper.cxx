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
#include "vtkPVConfig.h"
#include "vtkPVInitializer.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerInterpreterInitializer.h"
#include "vtkOutputWindow.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkSmartPointer.h"
#include "vtkSMMessage.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"

#include <string>
#include <vtksys/ios/sstream>

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

  vtkProcessModule::GetProcessModule()->SetOptions(options);

  // this has to happen after process module is initialized and options have
  // been set.
  PARAVIEW_INITIALIZE();

  // Set multi-server flag to vtkProcessModule
  vtkProcessModule::GetProcessModule()->SetMultipleSessionsSupport(
        options->GetMultiServerMode() != 0);

  // Make sure the ProxyManager get created...
  vtkSMProxyManager::GetProxyManager();
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
void vtkInitializationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
