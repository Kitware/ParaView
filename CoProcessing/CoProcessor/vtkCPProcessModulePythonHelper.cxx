/*=========================================================================

  Program:   ParaView
  Module:    vtkCPProcessModulePythonHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h" // include Python.h before other headers
#include "vtkCPProcessModulePythonHelper.h"

#include "CPSystemInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVPythonInterpretor.h"
#include "vtkPVPythonOptions.h"
#include "vtkSMApplication.h"
#include "vtkSMProperty.h"

#define EXCLUDE_LOAD_ALL_FUNCTION
#include "cppythonmodules.h"

#include <vtkstd/string>
using namespace vtkstd;

vtkStandardNewMacro(vtkCPProcessModulePythonHelper);

//----------------------------------------------------------------------------
vtkCPProcessModulePythonHelper::vtkCPProcessModulePythonHelper()
{
  this->Interpretor = 0;
}

//----------------------------------------------------------------------------
vtkCPProcessModulePythonHelper::~vtkCPProcessModulePythonHelper()
{
  // Cleanup the python interpretor.
  if (this->Interpretor)
    {
    this->Interpretor->Delete();
    this->Interpretor=0;
    }
}

//----------------------------------------------------------------------------
void vtkCPProcessModulePythonHelper::PrintSelf(
    ostream& os,
    vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Interpretor: " << this->Interpretor << "\n";
}

//----------------------------------------------------------------------------
int vtkCPProcessModulePythonHelper::RunGUIStart(
    int vtkNotUsed(argc),
    char **argv,
    int numServerProcs,
    int myId)
{
  vtkPVPythonOptions* boptions = vtkPVPythonOptions::SafeDownCast(
    this->ProcessModule->GetOptions());
  if (myId > 0 && !boptions->GetSymmetricMPIMode())
    {
    return 0;
    }

  (void)numServerProcs;

  this->SMApplication->Initialize();
  vtkSMProperty::SetCheckDomains(0);

  // Do static initialization of python libraries
  cppythonmodules_h_LoadAllPythonModules();

  // Initialize the sub-interpreter because that is where RunSimpleString
  // works.
  this->Interpretor=vtkPVPythonInterpretor::New();
  int interpOk = this->Interpretor->InitializeSubInterpretor(1, argv);

#ifdef PARAVIEW_INSTALL_DIR
  this->Interpretor->AddPythonPath(PARAVIEW_INSTALL_DIR "/bin/" COPROCESSOR_BUILD_DIR);
  this->Interpretor->AddPythonPath(PARAVIEW_INSTALL_DIR "/Utilities/VTKPythonWrapping");
#else
  vtkErrorMacro("ParaView install directory is undefined.");
  return 0;
#endif

  return interpOk;
}

