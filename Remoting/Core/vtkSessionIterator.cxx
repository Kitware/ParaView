// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSessionIterator.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleInternals.h"

#include <cassert>

class vtkSessionIterator::vtkInternals
{
public:
  vtkProcessModuleInternals::MapOfSessions::iterator Iter;
};

vtkStandardNewMacro(vtkSessionIterator);
//----------------------------------------------------------------------------
vtkSessionIterator::vtkSessionIterator()
{
  this->Internals = new vtkInternals();
  this->InitTraversal();
}

//----------------------------------------------------------------------------
vtkSessionIterator::~vtkSessionIterator()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkSessionIterator::InitTraversal()
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
bool vtkSessionIterator::IsDoneWithTraversal()
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
void vtkSessionIterator::GoToNextItem()
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
vtkSession* vtkSessionIterator::GetCurrentSession()
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
vtkIdType vtkSessionIterator::GetCurrentSessionId()
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
void vtkSessionIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
