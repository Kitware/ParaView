// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMAMRLevelsDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

vtkStandardNewMacro(vtkSMAMRLevelsDomain);

//----------------------------------------------------------------------------
vtkSMAMRLevelsDomain::vtkSMAMRLevelsDomain() = default;
vtkSMAMRLevelsDomain::~vtkSMAMRLevelsDomain() = default;

//----------------------------------------------------------------------------
void vtkSMAMRLevelsDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSMAMRLevelsDomain::Update(vtkSMProperty*)
{
  auto* dataInfo = this->GetInputInformation();
  int numberOfLevels = dataInfo ? dataInfo->GetNumberOfAMRLevels() : 0;
  std::vector<vtkEntry> entries;
  entries.emplace_back(0, numberOfLevels - 1);
  this->SetEntries(entries);
}

//---------------------------------------------------------------------------
vtkPVDataInformation* vtkSMAMRLevelsDomain::GetInputInformation()
{
  vtkSMProperty* inputProperty = this->GetRequiredProperty("Input");
  if (!inputProperty)
  {
    vtkErrorMacro("Missing required property with function 'Input'");
    return nullptr;
  }

  vtkSMUncheckedPropertyHelper helper(inputProperty);
  if (helper.GetNumberOfElements() > 0)
  {
    vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(helper.GetAsProxy(0));
    if (sp)
    {
      return sp->GetDataInformation(helper.GetOutputPort());
    }
  }
  return nullptr;
}
