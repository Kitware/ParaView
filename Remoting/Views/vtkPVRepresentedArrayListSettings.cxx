/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRepresentedArrayListSettings.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRepresentedArrayListSettings.h"

#include "vtkDataArraySelection.h"
#include "vtkObjectFactory.h"
#include "vtkSMProxyManager.h"
#include "vtkSMReaderFactory.h"
#include "vtkSMSession.h"
#include "vtkStringArray.h"

#include <cassert>
#include <string>
#include <vector>

vtkSmartPointer<vtkPVRepresentedArrayListSettings> vtkPVRepresentedArrayListSettings::Instance;

class vtkPVRepresentedArrayListSettings::vtkInternals
{
public:
  std::vector<std::string> FilterExpressions;
  std::vector<std::string> ExcludedNameFilters;
  vtkNew<vtkStringArray> AllNameFilters;
};

//----------------------------------------------------------------------------
vtkPVRepresentedArrayListSettings* vtkPVRepresentedArrayListSettings::New()
{
  vtkPVRepresentedArrayListSettings* instance = vtkPVRepresentedArrayListSettings::GetInstance();
  assert(instance);
  instance->Register(nullptr);
  return instance;
}

//----------------------------------------------------------------------------
vtkPVRepresentedArrayListSettings* vtkPVRepresentedArrayListSettings::GetInstance()
{
  if (!vtkPVRepresentedArrayListSettings::Instance)
  {
    vtkPVRepresentedArrayListSettings* instance = new vtkPVRepresentedArrayListSettings();
    instance->InitializeObjectBase();
    vtkPVRepresentedArrayListSettings::Instance.TakeReference(instance);
  }
  return vtkPVRepresentedArrayListSettings::Instance;
}

//----------------------------------------------------------------------------
vtkPVRepresentedArrayListSettings::vtkPVRepresentedArrayListSettings()
{
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkPVRepresentedArrayListSettings::~vtkPVRepresentedArrayListSettings()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkPVRepresentedArrayListSettings::SetNumberOfFilterExpressions(int n)
{
  if (n != this->GetNumberOfFilterExpressions())
  {
    this->Internals->FilterExpressions.resize(n);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkPVRepresentedArrayListSettings::GetNumberOfFilterExpressions()
{
  return static_cast<int>(this->Internals->FilterExpressions.size());
}

//----------------------------------------------------------------------------
void vtkPVRepresentedArrayListSettings::SetFilterExpression(int i, const char* expression)
{
  if (i >= 0 && i < this->GetNumberOfFilterExpressions())
  {
    if (strcmp(this->Internals->FilterExpressions[i].c_str(), expression) != 0)
    {
      this->Internals->FilterExpressions[i] = expression;
      this->Modified();
    }
  }
  else
  {
    vtkErrorMacro("Index out of range: " << i);
  }
}

//----------------------------------------------------------------------------
const char* vtkPVRepresentedArrayListSettings::GetFilterExpression(int i)
{
  if (i >= 0 && i < this->GetNumberOfFilterExpressions())
  {
    return this->Internals->FilterExpressions[i].c_str();
  }
  else
  {
    vtkErrorMacro("Index out of range: " << i);
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkPVRepresentedArrayListSettings::SetNumberOfExcludedNameFilters(int n)
{
  if (n != this->GetNumberOfExcludedNameFilters())
  {
    this->Internals->ExcludedNameFilters.resize(n);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkPVRepresentedArrayListSettings::GetNumberOfExcludedNameFilters()
{
  return static_cast<int>(this->Internals->ExcludedNameFilters.size());
}

//----------------------------------------------------------------------------
void vtkPVRepresentedArrayListSettings::SetExcludedNameFilter(int i, const char* expression)
{
  if (i >= 0 && i < this->GetNumberOfExcludedNameFilters())
  {
    if (strcmp(this->Internals->ExcludedNameFilters[i].c_str(), expression) != 0)
    {
      this->Internals->ExcludedNameFilters[i] = expression;
      this->Modified();
    }
  }
  else
  {
    vtkErrorMacro("Index out of range: " << i);
  }
}

//----------------------------------------------------------------------------
const char* vtkPVRepresentedArrayListSettings::GetExcludedNameFilter(int i)
{
  if (i >= 0 && i < this->GetNumberOfExcludedNameFilters())
  {
    return this->Internals->ExcludedNameFilters[i].c_str();
  }
  else
  {
    vtkErrorMacro("Index out of range: " << i);
  }

  return nullptr;
}

//------------------------------------------------------------------------------
vtkStringArray* vtkPVRepresentedArrayListSettings::GetAllNameFilters()
{
  return this->Internals->AllNameFilters;
}

//----------------------------------------------------------------------------
void vtkPVRepresentedArrayListSettings::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
