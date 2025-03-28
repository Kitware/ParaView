// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMArraySelectionDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVGeneralSettings.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMVectorProperty.h"

vtkStandardNewMacro(vtkSMArraySelectionDomain);

//---------------------------------------------------------------------------
vtkSMArraySelectionDomain::vtkSMArraySelectionDomain() = default;

//---------------------------------------------------------------------------
vtkSMArraySelectionDomain::~vtkSMArraySelectionDomain() = default;

//---------------------------------------------------------------------------
int vtkSMArraySelectionDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values)
{
  vtkSMVectorProperty* vprop = vtkSMVectorProperty::SafeDownCast(prop);
  vtkSMVectorProperty* infoProp = vtkSMVectorProperty::SafeDownCast(prop->GetInformationProperty());
  if (vprop && infoProp)
  {
    if (use_unchecked_values)
    {
      vtkWarningMacro("Developer Warnings: missing unchecked implementation.");
    }

    vprop->Copy(infoProp);

    if (vtkSMArraySelectionDomain::GetLoadAllVariables())
    {
      vtkSMPropertyHelper helper(vprop);

      for (unsigned int i = 0; i < this->GetNumberOfStrings(); i++)
      {
        vtkPVXMLElement* omitFromLoadAllVariablesHint =
          (prop->GetHints() ? prop->GetHints()->FindNestedElementByName("OmitFromLoadAllVariables")
                            : nullptr);
        if (!omitFromLoadAllVariablesHint)
        {
          helper.SetStatus(this->GetString(i), 1);
        }
      }
    }
    return 1;
  }
  return this->Superclass::SetDefaultValues(prop, use_unchecked_values);
}

//---------------------------------------------------------------------------
void vtkSMArraySelectionDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSMArraySelectionDomain::SetLoadAllVariables(bool choice)
{
  vtkPVGeneralSettings::GetInstance()->SetLoadAllVariables(choice);
}

//---------------------------------------------------------------------------
bool vtkSMArraySelectionDomain::GetLoadAllVariables()
{
  return vtkPVGeneralSettings::GetInstance()->GetLoadAllVariables();
}
