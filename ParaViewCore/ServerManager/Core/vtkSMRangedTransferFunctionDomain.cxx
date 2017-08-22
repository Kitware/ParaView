/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRangedTransferFunctionDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRangedTransferFunctionDomain.h"

#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMArrayRangeDomain.h"

class vtkSMRangedTransferFunctionDomainInternals
{
public:
  vtkSMRangedTransferFunctionDomainInternals() { this->ArrayRangeDomain = nullptr; }
  ~vtkSMRangedTransferFunctionDomainInternals()
  {
    if (this->ArrayRangeDomain != nullptr)
    {
      this->ArrayRangeDomain->Delete();
    }
  }

  vtkSMArrayRangeDomain* ArrayRangeDomain;
  unsigned long ObserverId;
};

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkSMRangedTransferFunctionDomain);
//-----------------------------------------------------------------------------
vtkSMRangedTransferFunctionDomain::vtkSMRangedTransferFunctionDomain()
{
  this->Internals = new vtkSMRangedTransferFunctionDomainInternals;
}

//-----------------------------------------------------------------------------
vtkSMRangedTransferFunctionDomain::~vtkSMRangedTransferFunctionDomain()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
double vtkSMRangedTransferFunctionDomain::GetRangeMinimum(unsigned int idx, int& exists)
{
  if (this->Internals->ArrayRangeDomain)
  {
    return this->Internals->ArrayRangeDomain->GetMinimum(idx, exists);
  }
  else
  {
    exists = 0;
    return 0;
  }
}

//-----------------------------------------------------------------------------
double vtkSMRangedTransferFunctionDomain::GetRangeMaximum(unsigned int idx, int& exists)
{
  if (this->Internals->ArrayRangeDomain)
  {
    return this->Internals->ArrayRangeDomain->GetMaximum(idx, exists);
  }
  else
  {
    exists = 0;
    return 0;
  }
}

//-----------------------------------------------------------------------------
bool vtkSMRangedTransferFunctionDomain::GetRangeMinimumExists(unsigned int idx)
{
  if (this->Internals->ArrayRangeDomain)
  {
    return (this->Internals->ArrayRangeDomain->GetMaximumExists(idx) != 0);
  }
  else
  {
    return false;
  }
}

//-----------------------------------------------------------------------------
bool vtkSMRangedTransferFunctionDomain::GetRangeMaximumExists(unsigned int idx)
{
  if (this->Internals->ArrayRangeDomain)
  {
    return (this->Internals->ArrayRangeDomain->GetMaximumExists(idx) != 0);
  }
  else
  {
    return false;
  }
}

//-----------------------------------------------------------------------------
int vtkSMRangedTransferFunctionDomain::ReadXMLAttributes(
  vtkSMProperty* prop, vtkPVXMLElement* element)
{
  this->AddProxy("piecewise_functions", "PiecewiseFunction");
  if (this->Internals->ArrayRangeDomain != nullptr)
  {
    this->Internals->ArrayRangeDomain->RemoveObserver(this->Internals->ObserverId);
    this->Internals->ArrayRangeDomain->Delete();
  }
  this->Internals->ArrayRangeDomain = vtkSMArrayRangeDomain::New();
  this->Internals->ObserverId =
    this->Internals->ArrayRangeDomain->AddObserver(vtkCommand::DomainModifiedEvent, this,
      &vtkSMRangedTransferFunctionDomain::InvokeDomainModifiedEvent);
  return this->Internals->ArrayRangeDomain->ReadXMLAttributes(prop, element) ? 1 : 0;
}

//---------------------------------------------------------------------------
void vtkSMRangedTransferFunctionDomain::InvokeDomainModifiedEvent()
{
  this->InvokeEvent(vtkCommand::DomainModifiedEvent);
}

//-----------------------------------------------------------------------------
void vtkSMRangedTransferFunctionDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
