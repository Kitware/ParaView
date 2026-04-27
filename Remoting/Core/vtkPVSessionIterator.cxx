// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVSessionIterator.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleInternals.h"

#include <cassert>

class vtkPVSessionIterator::vtkInternals
{
public:
  vtkProcessModuleInternals::MapOfSessions::iterator Iter;
};

vtkStandardNewMacro(vtkPVSessionIterator);
//----------------------------------------------------------------------------
vtkPVSessionIterator::vtkPVSessionIterator()
{
  this->Internals = new vtkInternals();
  this->InitTraversal();
}

//----------------------------------------------------------------------------
vtkPVSessionIterator::~vtkPVSessionIterator()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkPVSessionIterator::InitTraversal()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
  {
    vtkErrorMacro("No ProcessModule found.");
    return;
  }

  vtkProcessModuleInternals* internals = pm->Internals;
  this->Internals->Iter = internals->Sessions.begin();
}

//----------------------------------------------------------------------------
bool vtkPVSessionIterator::IsDoneWithTraversal()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
  {
    vtkErrorMacro("No ProcessModule found.");
    return true;
  }
  vtkProcessModuleInternals* internals = pm->Internals;
  return (this->Internals->Iter == internals->Sessions.end());
}

//----------------------------------------------------------------------------
void vtkPVSessionIterator::GoToNextItem()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
  {
    vtkErrorMacro("No ProcessModule found.");
    return;
  }
  this->Internals->Iter++;
}

//----------------------------------------------------------------------------
vtkPVSession* vtkPVSessionIterator::GetCurrentSession()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
  {
    vtkErrorMacro("No ProcessModule found.");
    return nullptr;
  }

  assert(this->IsDoneWithTraversal() == false);
  return this->Internals->Iter->second.GetPointer();
}

//----------------------------------------------------------------------------
vtkIdType vtkPVSessionIterator::GetCurrentSessionId()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
  {
    vtkErrorMacro("No ProcessModule found.");
    return 0;
  }

  assert(this->IsDoneWithTraversal() == false);
  return this->Internals->Iter->first;
}

//----------------------------------------------------------------------------
void vtkPVSessionIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
