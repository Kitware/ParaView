/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSessionIterator.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule2.h"
#include "vtkProcessModule2Internals.h"

#include <assert.h>

class vtkSessionIterator::vtkInternals
{
public:
  vtkProcessModule2::vtkInternals::MapOfSessions::iterator Iter;
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
  vtkProcessModule2* pm = vtkProcessModule2::GetProcessModule();
  if (!pm)
    {
    vtkErrorMacro("No ProcessModule found.");
    return;
    }

  vtkProcessModule2::vtkInternals* internals = pm->Internals;
  this->Internals->Iter = internals->Sessions.begin();
}

//----------------------------------------------------------------------------
bool vtkSessionIterator::IsDoneWithTraversal()
{
  vtkProcessModule2* pm = vtkProcessModule2::GetProcessModule();
  if (!pm)
    {
    vtkErrorMacro("No ProcessModule found.");
    return true;
    }
  vtkProcessModule2::vtkInternals* internals = pm->Internals;
  return (this->Internals->Iter == internals->Sessions.end());
}

//----------------------------------------------------------------------------
void vtkSessionIterator::GoToNextItem()
{
  vtkProcessModule2* pm = vtkProcessModule2::GetProcessModule();
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
  vtkProcessModule2* pm = vtkProcessModule2::GetProcessModule();
  if (!pm)
    {
    vtkErrorMacro("No ProcessModule found.");
    return NULL;
    }

  assert(this->IsDoneWithTraversal() == false);
  return this->Internals->Iter->second.GetPointer();
}

//----------------------------------------------------------------------------
vtkIdType vtkSessionIterator::GetCurrentSessionId()
{
  vtkProcessModule2* pm = vtkProcessModule2::GetProcessModule();
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
