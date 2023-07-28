// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkPVPythonModule.h"

#include "vtkDebugLeaksManager.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <cstring>
#include <list>

//=============================================================================
// The static structure holding all of the registered modules.
typedef std::list<vtkSmartPointer<vtkPVPythonModule>> vtkPVPythonModuleContainerType;
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
