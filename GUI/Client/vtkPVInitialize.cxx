/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInitialize.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVInitialize.h"

#include "vtkObjectFactory.h"
#include "vtkPVWindow.h"

#include "vtkToolkits.h" // Needed for vtkPVGeneratedModules
#include "vtkPVConfig.h" // Needed for vtkPVGeneratedModules
#include "vtkPVGeneratedModules.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVInitialize);
vtkCxxRevisionMacro(vtkPVInitialize, "1.7");

//----------------------------------------------------------------------------
vtkPVInitialize::vtkPVInitialize()
{
}

//----------------------------------------------------------------------------
vtkPVInitialize::~vtkPVInitialize()
{
}

//----------------------------------------------------------------------------
void vtkPVInitialize::Initialize(vtkPVWindow* win)
{
  char* init_string = vtkPVDefaultModulesGetInterfaces();
  win->ReadSourceInterfacesFromString(init_string);
  delete [] init_string;
}

//----------------------------------------------------------------------------
void vtkPVInitialize::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
