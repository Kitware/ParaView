/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSubsetInclusionLatticeDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSubsetInclusionLatticeDomain.h"

#include "vtkClientServerStreamInstantiator.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVSILInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkSubsetInclusionLattice.h"

vtkStandardNewMacro(vtkSMSubsetInclusionLatticeDomain);
//----------------------------------------------------------------------------
vtkSMSubsetInclusionLatticeDomain::vtkSMSubsetInclusionLatticeDomain()
{
  this->TimeTag = 0;
  this->SIL = nullptr;
}

//----------------------------------------------------------------------------
vtkSMSubsetInclusionLatticeDomain::~vtkSMSubsetInclusionLatticeDomain()
{
}

//----------------------------------------------------------------------------
void vtkSMSubsetInclusionLatticeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSMSubsetInclusionLatticeDomain::Update(vtkSMProperty*)
{
  vtkSMProperty* tsProperty = this->GetRequiredProperty("TimeStamp");
  if (tsProperty)
  {
    vtkSMUncheckedPropertyHelper helper(tsProperty);
    if (helper.GetAsIdType() != this->TimeTag)
    {
      vtkNew<vtkPVSILInformation> info;
      tsProperty->GetParent()->GatherInformation(info.Get());
      this->SIL->DeepCopy(info->GetSubsetInclusionLattice());

      std::vector<std::string> strings;
      auto selmap = this->SIL->GetSelection();
      for (auto iter : selmap)
      {
        strings.push_back(iter.first);
      }
      this->SetStrings(strings);
    }
  }
}

//----------------------------------------------------------------------------
vtkSubsetInclusionLattice* vtkSMSubsetInclusionLatticeDomain::GetSIL()
{
  return this->SIL;
}

//----------------------------------------------------------------------------
int vtkSMSubsetInclusionLatticeDomain::SetDefaultValues(
  vtkSMProperty* prop, bool use_unchecked_values)
{
  auto svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (!svp)
  {
    return 0;
  }

  vtkSubsetInclusionLattice* sil = this->GetSIL();
  auto oldSel = sil->GetSelection();
  sil->ClearSelections();

  if (!this->DefaultPath.empty())
  {
    sil->SelectAll(this->DefaultPath.c_str());
  }

  std::vector<std::string> strings;
  auto selmap = this->SIL->GetSelection();
  for (auto iter : selmap)
  {
    strings.push_back(iter.first);
    strings.push_back(iter.second ? "1" : "0");
  }
  if (use_unchecked_values)
  {
    svp->SetUncheckedElements(strings);
  }
  else
  {
    svp->SetElements(strings);
  }
  sil->SetSelection(oldSel);
  return 1;
}

//----------------------------------------------------------------------------
int vtkSMSubsetInclusionLatticeDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* elem)
{
  if (!this->Superclass::ReadXMLAttributes(prop, elem))
  {
    return 0;
  }

  if (const char* default_path = elem->GetAttribute("default_path"))
  {
    this->DefaultPath = default_path;
  }

  if (const char* sil_class = elem->GetAttribute("class"))
  {
    vtkSmartPointer<vtkObjectBase> anObject;
    anObject.TakeReference(vtkClientServerStreamInstantiator::CreateInstance(sil_class));
    this->SIL = vtkSubsetInclusionLattice::SafeDownCast(anObject);
  }
  if (this->SIL == nullptr)
  {
    this->SIL = vtkSmartPointer<vtkSubsetInclusionLattice>::New();
  }

  if (this->GetRequiredProperty("TimeStamp") == nullptr)
  {
    vtkWarningMacro(
      "Missing `TimeStamp` required property. The domain may not function as expected.");
  }
  return 1;
}
