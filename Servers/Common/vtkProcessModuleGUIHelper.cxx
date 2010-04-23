/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessModuleGUIHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProcessModuleGUIHelper.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"

//-----------------------------------------------------------------------------
vtkProcessModuleGUIHelper::vtkProcessModuleGUIHelper()
{
  this->ProcessModule = 0;
}

//-----------------------------------------------------------------------------
int vtkProcessModuleGUIHelper::Run(vtkPVOptions* options)
{
  if (!this->ProcessModule)
    {
    vtkErrorMacro("ProcessModule must be set before calling Start().");
    return 1;
    }

  int new_argc = 0;
  char** new_argv = 0;
  options->GetRemainingArguments(&new_argc, &new_argv);
 return this->ProcessModule->Start(new_argc, new_argv); 
}

//-----------------------------------------------------------------------------
void vtkProcessModuleGUIHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
void vtkProcessModuleGUIHelper::SetProcessModule(vtkProcessModule* pm)
{
  this->ProcessModule = pm;
}


