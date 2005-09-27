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

vtkCxxRevisionMacro(vtkProcessModuleGUIHelper, "1.2");

vtkProcessModuleGUIHelper::vtkProcessModuleGUIHelper()
{
  this->ProcessModule = 0;
}

void vtkProcessModuleGUIHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

void vtkProcessModuleGUIHelper::SetProcessModule(vtkProcessModule* pm)
{
  this->ProcessModule = pm;
}


