/*=========================================================================

  Module:    vtkPVGUIClientOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGUIClientOptions.h"

#include "vtkObjectFactory.h"

#include <kwsys/CommandLineArguments.hxx>
#include <kwsys/SystemTools.hxx>


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVGUIClientOptions);
vtkCxxRevisionMacro(vtkPVGUIClientOptions, "1.6");

//----------------------------------------------------------------------------
vtkPVGUIClientOptions::vtkPVGUIClientOptions()
{
  this->PlayDemoFlag = 0;
  this->DisableRegistry = 0;
  this->CrashOnErrors = 0;
  this->StartEmpty = 0;
  this->ParaViewScriptName = 0;
  this->ParaViewDataName = 0;
  this->SetProcessType(vtkPVOptions::PARAVIEW);
}

//----------------------------------------------------------------------------
vtkPVGUIClientOptions::~vtkPVGUIClientOptions()
{
  this->SetParaViewScriptName(0);
  this->SetParaViewDataName(0);
}

//----------------------------------------------------------------------------
void vtkPVGUIClientOptions::Initialize()
{
  this->Superclass::Initialize();
  this->AddArgument("--data", 0, &this->ParaViewDataName,
                    "Load the specified data.");
  this->AddBooleanArgument("--play-demo", "-pd", &this->PlayDemoFlag,
                           "Run the ParaView demo.");
  this->AddBooleanArgument("--disable-registry", "-dr", &this->DisableRegistry,
                           "Do not use registry when running ParaView (for testing).");
  this->AddBooleanArgument("--crash-on-errors", 0, &this->CrashOnErrors, 
                           "For debugging purposes. This will make ParaView abort on errors.");
  this->AddBooleanArgument("--start-empty", "-e", &this->StartEmpty, 
                           "Start ParaView without any default modules.");

  // Add deprecated command line arguments for paraview
  this->AddDeprecatedArgument("--server", "-v", 
                              "Deprecated. Use pvserver executable for this function now.", 
                              vtkPVOptions::PARAVIEW); 
  this->AddDeprecatedArgument("--render-server", "-rs", 
                              "Deprecated. Use pvrenderserver.", 
                              vtkPVOptions::PARAVIEW);
  this->AddDeprecatedArgument("--client", "-c", 
                              "Deprecated. Use pvclient executable for this function now.",
                              vtkPVOptions::PARAVIEW);
  this->AddDeprecatedArgument("--cave-configuration", "-cc", 
                              "Deprecated. The cave configuration must now be specified in the xml .pvx file",
                              vtkPVOptions::PARAVIEW);
  this->AddDeprecatedArgument("--host", "-h",
                              "Deprecated. Use --client-host, --data-server-host, --render-server-host",
                              vtkPVOptions::PARAVIEW);
  this->AddDeprecatedArgument("--port", 0, "Deprecated. Use --client-port, --data-port, --render-port.",
                              vtkPVOptions::PARAVIEW);
  this->AddDeprecatedArgument("--machines",
                              "-m", 
                              "Deprecated. Use xml pvx file to specify machines for render and data servers.",
                              vtkPVOptions::PARAVIEW);
  this->AddDeprecatedArgument("--use-software-rendering", "-r", 
                              "Deprecated. This option is no longer available.", vtkPVOptions::PARAVIEW);
  this->AddDeprecatedArgument("--use-satellite-rendering", "-s",  
                              "Deprecated. This option is no longer available.", vtkPVOptions::PARAVIEW);

}

//----------------------------------------------------------------------------
int vtkPVGUIClientOptions::PostProcess(int argc, const char* const* argv)
{
  return this->Superclass::PostProcess(argc, argv);
}

int vtkPVGUIClientOptions::WrongArgument(const char* argument)
{
  if ( kwsys::SystemTools::FileExists(argument) &&
    kwsys::SystemTools::GetFilenameLastExtension(argument) == ".pvs")
    {
    this->SetParaViewScriptName(argument);
    return 1;
    }

  return this->Superclass::WrongArgument(argument);
}

//----------------------------------------------------------------------------
void vtkPVGUIClientOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PlayDemoFlag: " << this->PlayDemoFlag << endl;
  os << indent << "DisableRegistry: " << this->DisableRegistry << endl;
  os << indent << "CrashOnErrors: " << this->CrashOnErrors << endl;
  os << indent << "StartEmpty: " << this->StartEmpty << endl;
  os << indent << "ParaViewScriptName: " << (this->ParaViewScriptName?this->ParaViewScriptName:"(none)") << endl;
  os << indent << "ParaViewDataName: " << (this->ParaViewDataName?this->ParaViewDataName:"(none)") << endl;
}
