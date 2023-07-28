// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMPrismIsSimulationDataDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"

//=============================================================================
vtkStandardNewMacro(vtkSMPrismIsSimulationDataDomain);

//-----------------------------------------------------------------------------
vtkSMPrismIsSimulationDataDomain::vtkSMPrismIsSimulationDataDomain() = default;

//-----------------------------------------------------------------------------
vtkSMPrismIsSimulationDataDomain::~vtkSMPrismIsSimulationDataDomain() = default;

//-----------------------------------------------------------------------------
void vtkSMPrismIsSimulationDataDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IsSimulationData: " << this->IsSimulationData << endl;
}

//-----------------------------------------------------------------------------
void vtkSMPrismIsSimulationDataDomain::Update(vtkSMProperty*)
{
  vtkSMProperty* input = this->GetRequiredProperty("Input");
  if (!input)
  {
    vtkErrorMacro("Missing require property 'Input'. Update failed.");
    return;
  }
  vtkPVDataInformation* dataInfo = this->GetInputDataInformation("Input");
  if (!dataInfo)
  {
    return;
  }
  auto arrayInformation = dataInfo->GetArrayInformation("PRISM_DATA", vtkDataObject::FIELD);
  this->IsSimulationData = arrayInformation == nullptr;
}

//-----------------------------------------------------------------------------
int vtkSMPrismIsSimulationDataDomain::SetDefaultValues(
  vtkSMProperty* prop, bool use_unchecked_values)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(prop);
  if (!ivp)
  {
    vtkErrorMacro("Property is not a vtkSMIntVectorProperty.");
    return 0;
  }
  vtkSMPropertyHelper helper(ivp);
  helper.SetUseUnchecked(use_unchecked_values);
  helper.Set(0, this->IsSimulationData);
  return this->Superclass::SetDefaultValues(prop, use_unchecked_values);
}
