// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMInputFileNameDomain.h"
#include "vtkSMSourceProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkSmartPointer.h"

#include <iostream>
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkSMInputFileNameDomain);

//---------------------------------------------------------------------------
vtkSMInputFileNameDomain::vtkSMInputFileNameDomain() = default;

//---------------------------------------------------------------------------
vtkSMInputFileNameDomain::~vtkSMInputFileNameDomain() = default;

//---------------------------------------------------------------------------
void vtkSMInputFileNameDomain::Update(vtkSMProperty* vtkNotUsed(prop))
{
  vtkSMProperty* inputProperty = this->GetRequiredProperty("Input");
  if (!inputProperty)
  {
    return;
  }

  vtkSMUncheckedPropertyHelper inputHelper(inputProperty);
  vtkSMProxy* inputProxy = inputHelper.GetAsProxy();
  if (inputProxy != nullptr)
  {
    const char* propertyName = vtkSMCoreUtilities::GetFileNameProperty(inputProxy);
    vtkSMProperty* fileNameProperty = inputProxy->GetProperty(propertyName);
    if (fileNameProperty != nullptr)
    {
      std::string fname(vtkSMPropertyHelper(fileNameProperty).GetAsString());
      this->FileName = fname;
      this->DomainModified();
      return;
    }
  }
}

//---------------------------------------------------------------------------
int vtkSMInputFileNameDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (svp && !this->FileName.empty())
  {
    if (use_unchecked_values)
    {
      svp->SetUncheckedElement(0, this->FileName.c_str());
    }
    else
    {
      svp->SetElement(0, this->FileName.c_str());
    }
    return 1;
  }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMInputFileNameDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << this->FileName << endl;
}
