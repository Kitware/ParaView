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
#include "vtkSMProxyProperty.h"
#include "vtkSMTransferFunctionProxy.h"

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

//-----------------------------------------------------------------------------
int vtkSMRangedTransferFunctionDomain::SetDefaultValues(
  vtkSMProperty* prop, bool use_unchecked_values)
{
  vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(prop);
  vtkSMTransferFunctionProxy* transferFunctionProxy = nullptr;
  if (proxyProp)
  {
    if (use_unchecked_values)
    {
      if (proxyProp->GetNumberOfUncheckedProxies() > 0)
      {
        transferFunctionProxy =
          vtkSMTransferFunctionProxy::SafeDownCast(proxyProp->GetUncheckedProxy(0));
      }
    }
    else
    {
      if (proxyProp->GetNumberOfProxies() > 0)
      {
        transferFunctionProxy = vtkSMTransferFunctionProxy::SafeDownCast(proxyProp->GetProxy(0));
      }
    }
  }

  if (transferFunctionProxy)
  {
    double rangeMin = 0.0;
    double rangeMax = 1.0;
    if (this->GetRangeMinimumExists(0) && this->GetRangeMaximumExists(0))
    {
      rangeMin = this->GetRangeMinimum(0);
      rangeMax = this->GetRangeMaximum(0);
    }
    else if (this->GetRangeMinimumExists(0))
    {
      rangeMin = rangeMax = this->GetRangeMinimum(0);
    }
    else if (this->GetRangeMaximumExists(0))
    {
      rangeMin = rangeMax = this->GetRangeMaximum(0);
    }
    transferFunctionProxy->RescaleTransferFunction(rangeMin, rangeMax, false);
  }
  return this->Superclass::SetDefaultValues(prop, use_unchecked_values);
}
