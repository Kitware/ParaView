/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPythonModule.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkPVPythonModule.h"

#include "vtkDebugLeaksManager.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <list>
#include <string.h>

//=============================================================================
// The static structure holding all of the registered modules.
typedef std::list<vtkSmartPointer<vtkPVPythonModule> > vtkPVPythonModuleContainerType;
static vtkPVPythonModuleContainerType vtkPVPythonModuleRegisteredModules;

void vtkPVPythonModule::RegisterModule(vtkPVPythonModule* module)
{
  vtkPVPythonModuleRegisteredModules.push_front(module);
}

vtkPVPythonModule* vtkPVPythonModule::GetModule(const char* fullname)
{
  vtkPVPythonModuleContainerType::iterator iter;
  for (iter = vtkPVPythonModuleRegisteredModules.begin();
       iter != vtkPVPythonModuleRegisteredModules.end(); iter++)
  {
    if (strcmp((*iter)->GetFullName(), fullname) == 0)
    {
      return *iter;
    }
  }
  return nullptr;
}

//=============================================================================
vtkStandardNewMacro(vtkPVPythonModule);

//-----------------------------------------------------------------------------
vtkPVPythonModule::vtkPVPythonModule()
{
  this->Source = nullptr;
  this->FullName = nullptr;
  this->IsPackage = 0;
}

vtkPVPythonModule::~vtkPVPythonModule()
{
  this->SetSource(nullptr);
  this->SetFullName(nullptr);
}

void vtkPVPythonModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FullName: " << this->FullName << endl;
  os << indent << "IsPackage: " << this->IsPackage << endl;
  os << indent << "Source: " << endl << this->Source << endl;
}
