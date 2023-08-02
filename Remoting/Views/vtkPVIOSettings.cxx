// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVIOSettings.h"

#include "vtkDataArraySelection.h"
#include "vtkObjectFactory.h"
#include "vtkSMProxyManager.h"
#include "vtkSMReaderFactory.h"
#include "vtkSMSession.h"
#include "vtkStringArray.h"

#include <cassert>
#include <string>
#include <vector>

class vtkPVIOSettings::vtkInternals
{
public:
  std::vector<std::string> ExcludedNameFilters;
  vtkNew<vtkStringArray> AllNameFilters;
};

//----------------------------------------------------------------------------
vtkPVIOSettings* vtkPVIOSettings::New()
{
  vtkPVIOSettings* instance = vtkPVIOSettings::GetInstance();
  assert(instance);
  instance->Register(nullptr);
  return instance;
}

//----------------------------------------------------------------------------
vtkPVIOSettings* vtkPVIOSettings::GetInstance()
{
  static vtkSmartPointer<vtkPVIOSettings> Instance;
  if (Instance.GetPointer() == nullptr)
  {
    vtkPVIOSettings* instance = new vtkPVIOSettings();
    instance->InitializeObjectBase();
    Instance.TakeReference(instance);
  }
  return Instance;
}

//----------------------------------------------------------------------------
vtkPVIOSettings::vtkPVIOSettings()
  : Internals(new vtkPVIOSettings::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkPVIOSettings::~vtkPVIOSettings() = default;

//----------------------------------------------------------------------------
void vtkPVIOSettings::SetNumberOfExcludedNameFilters(int n)
{
  if (n != this->GetNumberOfExcludedNameFilters())
  {
    this->Internals->ExcludedNameFilters.resize(n);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkPVIOSettings::GetNumberOfExcludedNameFilters()
{
  return static_cast<int>(this->Internals->ExcludedNameFilters.size());
}

//----------------------------------------------------------------------------
void vtkPVIOSettings::SetExcludedNameFilter(int i, const char* expression)
{
  if (i >= 0 && i < this->GetNumberOfExcludedNameFilters())
  {
    if (this->Internals->ExcludedNameFilters[i] != expression)
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
const char* vtkPVIOSettings::GetExcludedNameFilter(int i)
{
  if (i >= 0 && i < this->GetNumberOfExcludedNameFilters())
  {
    return this->Internals->ExcludedNameFilters[i].c_str();
  }
  vtkErrorMacro("Index out of range: " << i);
  return nullptr;
}

//------------------------------------------------------------------------------
vtkStringArray* vtkPVIOSettings::GetAllNameFilters()
{
  return this->Internals->AllNameFilters;
}

//----------------------------------------------------------------------------
void vtkPVIOSettings::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
