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
//#include "vtkPVDefaultModules.h"
#include "vtkPVGeneratedModules.h"
#include "vtkPVWindow.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVInitialize);
vtkCxxRevisionMacro(vtkPVInitialize, "1.6");

//----------------------------------------------------------------------------
vtkPVInitialize::vtkPVInitialize()
{
  this->StandardFiltersString = 0;
  this->StandardManipulatorsString = 0;
  this->StandardReadersString = 0;
  this->StandardSourcesString = 0;
  this->StandardWritersString = 0;
}

//----------------------------------------------------------------------------
vtkPVInitialize::~vtkPVInitialize()
{
  this->SetStandardFiltersString(0);
  this->SetStandardManipulatorsString(0);
  this->SetStandardReadersString(0);
  this->SetStandardSourcesString(0);
  this->SetStandardWritersString(0);
}

//----------------------------------------------------------------------------
void vtkPVInitialize::Initialize(vtkPVWindow* win)
{
  win->ReadSourceInterfacesFromString(this->GetStandardSourcesInterfaces());
  win->ReadSourceInterfacesFromString(this->GetStandardFiltersInterfaces());
  win->ReadSourceInterfacesFromString(this->GetStandardReadersInterfaces());
  win->ReadSourceInterfacesFromString(this->GetStandardManipulatorsInterfaces());
  win->ReadSourceInterfacesFromString(this->GetStandardWritersInterfaces());
}

//----------------------------------------------------------------------------
void vtkPVInitialize::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
