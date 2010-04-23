/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSubPropertyIterator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSubPropertyIterator.h"

#include "vtkObjectFactory.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyInternals.h"

vtkStandardNewMacro(vtkSMSubPropertyIterator);

struct vtkSMSubPropertyIteratorInternals
{
  vtkSMPropertyInternals::PropertyMap::iterator SubPropertyIterator;
};

//---------------------------------------------------------------------------
vtkSMSubPropertyIterator::vtkSMSubPropertyIterator()
{
  this->Property = 0;
  this->Internals = new vtkSMSubPropertyIteratorInternals;
}

//---------------------------------------------------------------------------
vtkSMSubPropertyIterator::~vtkSMSubPropertyIterator()
{
  this->SetProperty(0);
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMSubPropertyIterator::SetProperty(vtkSMProperty* property)
{
  if (this->Property != property)
    {
    if (this->Property != NULL) { this->Property->UnRegister(this); }
    this->Property = property;
    if (this->Property != NULL) 
      { 
      this->Property->Register(this); 
      this->Begin();
      }
    this->Modified();
    }
}

//---------------------------------------------------------------------------
void vtkSMSubPropertyIterator::Begin()
{
  if (!this->Property)
    {
    vtkErrorMacro("Property is not set. Can not perform operation: Begin()");
    return;
    }

  this->Internals->SubPropertyIterator = 
    this->Property->PInternals->SubProperties.begin(); 
}

//---------------------------------------------------------------------------
int vtkSMSubPropertyIterator::IsAtEnd()
{
  if (!this->Property)
    {
    vtkErrorMacro("Property is not set. Can not perform operation: IsAtEnd()");
    return 1;
    }
  if ( this->Internals->SubPropertyIterator == 
       this->Property->PInternals->SubProperties.end() )
    {
    return 1;
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMSubPropertyIterator::Next()
{
  if (!this->Property)
    {
    vtkErrorMacro("Property is not set. Can not perform operation: Next()");
    return;
    }

  if (this->Internals->SubPropertyIterator != 
      this->Property->PInternals->SubProperties.end())
    {
    this->Internals->SubPropertyIterator++;
    return;
    }

}

//---------------------------------------------------------------------------
const char* vtkSMSubPropertyIterator::GetKey()
{
  if (!this->Property)
    {
    vtkErrorMacro("Property is not set. Can not perform operation: GetKey()");
    return 0;
    }

  if (this->Internals->SubPropertyIterator != 
      this->Property->PInternals->SubProperties.end())
    {
    return this->Internals->SubPropertyIterator->first.c_str();
    }

  return 0;
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMSubPropertyIterator::GetSubProperty()
{
  if (!this->Property)
    {
    vtkErrorMacro("Property is not set. Can not perform operation: GetSubProperty()");
    return 0;
    }
  if (this->Internals->SubPropertyIterator != 
      this->Property->PInternals->SubProperties.end())
    {
    return this->Internals->SubPropertyIterator->second.GetPointer();
    }

  return 0;
}

//---------------------------------------------------------------------------
void vtkSMSubPropertyIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Property: " << this->Property << endl;
}
