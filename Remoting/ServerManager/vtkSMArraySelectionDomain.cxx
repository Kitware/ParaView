// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMArraySelectionDomain.h"

#include "vtkCellTypeUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
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

    vtkSMPropertyHelper helper(vprop);
    if (infoProp->GetNumberOfElements() / 2 != this->GetNumberOfStrings())
    {
      for (unsigned int i = 0; i < this->GetNumberOfStrings(); i++)
      {
        helper.SetStatus(this->GetString(i), 0);
      }
    }
    else
    {
      vprop->Copy(infoProp);
    }

    if (vtkSMArraySelectionDomain::GetLoadAllVariables())
    {
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

//--------------------------------------------------------------------------
void vtkSMArraySelectionDomain::Update(vtkSMProperty* prop)
{
  this->Superclass::Update(prop);
  if (!this->UseCellTypes)
  {
    return;
  }

  vtkPVDataInformation* inputInfo = this->GetInputDataSetInformation("Input");
  // if xml attribute
  if (inputInfo)
  {
    const auto& types = inputInfo->GetUniqueCellTypes();
    std::vector<std::string> typesName;
    typesName.reserve(types.size());
    for (auto type : types)
    {
      typesName.push_back(vtkCellTypeUtilities::GetTypeAsString(type));
    }
    this->SetStrings(typesName);
  }
}

//---------------------------------------------------------------------------
void vtkSMArraySelectionDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseCellTypes: " << (this->UseCellTypes ? "On" : "Off") << endl;
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

//---------------------------------------------------------------------------
int vtkSMArraySelectionDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  // Search for attribute type with matching name.
  const char* mode = element->GetAttribute("mode");
  this->UseCellTypes = (mode && strcmp(mode, "cell_types") == 0);

  return this->Superclass::ReadXMLAttributes(prop, element);
}
