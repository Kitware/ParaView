// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSMDoubleMapPropertyIterator.h"

#include <map>
#include <vector>

#include "vtkObjectFactory.h"
#include "vtkSMDoubleMapProperty.h"

class vtkSMDoubleMapPropertyIteratorInternals
{
public:
  std::map<vtkIdType, std::vector<double>>* Map;
  std::map<vtkIdType, std::vector<double>>::iterator MapIterator;
};

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMDoubleMapPropertyIterator);

//---------------------------------------------------------------------------
vtkSMDoubleMapPropertyIterator::vtkSMDoubleMapPropertyIterator()
{
  this->Property = nullptr;
  this->Internals = new vtkSMDoubleMapPropertyIteratorInternals;
}

//---------------------------------------------------------------------------
vtkSMDoubleMapPropertyIterator::~vtkSMDoubleMapPropertyIterator()
{
  this->SetProperty(nullptr);
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMDoubleMapPropertyIterator::SetProperty(vtkSMDoubleMapProperty* property)
{
  if (this->Property != property)
  {
    if (this->Property != nullptr)
    {
      this->Property->UnRegister(this);
    }
    this->Property = property;
    if (this->Property != nullptr)
    {
      this->Property->Register(this);
      this->Begin();
    }
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSMDoubleMapPropertyIterator::Begin()
{
  if (!this->Property)
  {
    return;
  }

  this->Internals->Map =
    static_cast<std::map<vtkIdType, std::vector<double>>*>(this->Property->GetMapPointer());
  this->Internals->MapIterator = this->Internals->Map->begin();
}

//---------------------------------------------------------------------------
int vtkSMDoubleMapPropertyIterator::IsAtEnd()
{
  return this->Internals->MapIterator == this->Internals->Map->end();
}

//---------------------------------------------------------------------------
void vtkSMDoubleMapPropertyIterator::Next()
{
  this->Internals->MapIterator++;
}

//---------------------------------------------------------------------------
vtkIdType vtkSMDoubleMapPropertyIterator::GetKey()
{
  return this->Internals->MapIterator->first;
}

//---------------------------------------------------------------------------
void vtkSMDoubleMapPropertyIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
double vtkSMDoubleMapPropertyIterator::GetElementComponent(unsigned int component)
{
  return this->Internals->MapIterator->second[component];
}
