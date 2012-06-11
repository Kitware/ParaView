/*=========================================================================

  Program:   ParaView
  Module:    vtkCPCxxHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPCxxHelper.h"

#include "CPSystemInformation.h"
#include "vtkInitializationHelper.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h" // Required to get build options for paraview
#include "vtkPVOptions.h"
#include "vtkSMProxyManager.h"
#include "vtkSMObject.h"
#include "vtkSMProperty.h"

#include <string>
#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkCPCxxHelper);

vtkCPCxxHelper* vtkCPCxxHelper::Instance = 0;


//----------------------------------------------------------------------------
vtkCPCxxHelper::vtkCPCxxHelper()
{
  this->Options = 0;
}

//----------------------------------------------------------------------------
vtkCPCxxHelper::~vtkCPCxxHelper()
{
  if(this->Options)
    {
    this->Options->Delete();
    this->Options = 0;
    }
  vtkInitializationHelper::Finalize();
}

//----------------------------------------------------------------------------
vtkCPCxxHelper* vtkCPCxxHelper::New()
{
  if(vtkCPCxxHelper::Instance == 0)
    {
    // Try the factory first
    vtkCPCxxHelper::Instance = (vtkCPCxxHelper*)
      vtkObjectFactory::CreateInstance("vtkCPCxxHelper");
    // if the factory did not provide one, then create it here
    if(!vtkCPCxxHelper::Instance)
      {
      vtkCPCxxHelper::Instance = new vtkCPCxxHelper;
      }

    int argc = 1;
    char** argv = new char*[1];
    std::string CWD = vtksys::SystemTools::GetCurrentWorkingDirectory();
    argv[0] = new char[CWD.size()+1];
#ifdef COPROCESSOR_WIN32_BUILD
    strcpy_s(argv[0], strlen(CWD.c_str()), CWD.c_str());
#else
    strcpy(argv[0], CWD.c_str());
#endif

    vtkCPCxxHelper::Instance->Options = vtkPVOptions::New();
    vtkCPCxxHelper::Instance->Options->SetSymmetricMPIMode(1);
    vtkCPCxxHelper::Instance->Options->SetProcessType(vtkProcessModule::PROCESS_BATCH);

    vtkInitializationHelper::Initialize(
        argc, argv, vtkProcessModule::PROCESS_BATCH,
        vtkCPCxxHelper::Instance->Options);

    delete []argv[0];
    delete []argv;
    }

  return vtkCPCxxHelper::Instance;
}

//----------------------------------------------------------------------------
void vtkCPCxxHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
