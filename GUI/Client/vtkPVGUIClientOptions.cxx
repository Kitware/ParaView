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
//****************************************************************************
class vtkPVGUIClientOptionsInternal
{
public:
  kwsys::CommandLineArguments CMD;
};
//****************************************************************************
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVGUIClientOptions);
vtkCxxRevisionMacro(vtkPVGUIClientOptions, "1.1");

//----------------------------------------------------------------------------
vtkPVGUIClientOptions::vtkPVGUIClientOptions()
{
  this->PlayDemoFlag = 0;
  this->DisableRegistry = 0;
  this->CrashOnErrors = 0;
  this->StartEmpty = 0;
  this->ParaViewScriptName = 0;
  this->BatchScriptName = 0;

  // When running from client, default is different.
  this->ServerMode = 0;
}

//----------------------------------------------------------------------------
vtkPVGUIClientOptions::~vtkPVGUIClientOptions()
{
  this->SetParaViewScriptName(0);
  this->SetBatchScriptName(0);
}

//----------------------------------------------------------------------------
void vtkPVGUIClientOptions::Initialize()
{
  this->Superclass::Initialize();
  this->AddBooleanArgument("--server", "-v", &this->ServerMode,
    "Start ParaView as a server (use MPI run).");
  this->AddBooleanArgument("--client", "-c", &this->ClientMode,
    "Run ParaView as client (MPI run, 1 process) (ParaView Server must be started first).");
  this->AddArgument("--batch", "-b", &this->BatchScriptName,
    "Load and run the batch script specified.");
  this->AddBooleanArgument("--play-demo", "-pd", &this->PlayDemoFlag,
    "Run the ParaView demo.");
  this->AddBooleanArgument("--disable-registry", "-dr", &this->DisableRegistry,
    "Do not use registry when running ParaView (for testing).");
  this->AddBooleanArgument("--crash-on-errors", 0, &this->CrashOnErrors, 
    "For debugging purposes. This will make ParaView abort on errors.");
  this->AddBooleanArgument("--start-empty", "-e", &this->StartEmpty, 
    "Start ParaView without any default modules.");
}

//----------------------------------------------------------------------------
int vtkPVGUIClientOptions::PostProcess()
{
  if ( this->BatchScriptName && 
    kwsys::SystemTools::GetFilenameLastExtension(this->BatchScriptName) != ".pvb")
    {
    ostrstream str;
    str << "Wrong batch script name: " << this->BatchScriptName << ends;
    this->SetErrorMessage(str.str());
    str.rdbuf()->freeze(0);
    return 0;
    }
  return this->Superclass::PostProcess();
}

int vtkPVGUIClientOptions::WrongArgument(const char* argument)
{
  if ( kwsys::SystemTools::FileExists(argument) &&
    kwsys::SystemTools::GetFilenameLastExtension(argument) == ".pvs")
    {
    this->SetParaViewScriptName(argument);
    return 1;
    }
  if ( kwsys::SystemTools::FileExists(argument) &&
    kwsys::SystemTools::GetFilenameLastExtension(argument) == ".pvb")
    {
    this->SetBatchScriptName(argument);
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
  os << indent << "Port: " << this->Port<< endl;
  os << indent << "StartEmpty: " << this->StartEmpty << endl;
  os << indent << "Username: " << (this->Username?this->Username:"(none)") << endl;
  os << indent << "HostName: " << (this->HostName?this->HostName:"(none)") << endl;
  os << indent << "ParaViewScriptName: " << (this->ParaViewScriptName?this->ParaViewScriptName:"(none)") << endl;
  os << indent << "BatchScriptName: " << (this->BatchScriptName?this->BatchScriptName:"(none)") << endl;
}
