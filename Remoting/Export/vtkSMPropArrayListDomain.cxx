// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMPropArrayListDomain.h"

#include "vtkCompositeRepresentation.h"
#include "vtkDataSet.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVCompositeRepresentation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMRenderViewExporterProxy.h"
#include "vtkSMRenderViewProxy.h"

#include "vtkSMRepresentationProxy.h"
#include "vtkSMSourceProxy.h"

#include <vector>

vtkStandardNewMacro(vtkSMPropArrayListDomain);

//---------------------------------------------------------------------------
vtkSMPropArrayListDomain::vtkSMPropArrayListDomain()
{
  // vtkWarningMacro("Create prop domain");
}

//---------------------------------------------------------------------------
vtkSMPropArrayListDomain::~vtkSMPropArrayListDomain() {}

//---------------------------------------------------------------------------
void vtkSMPropArrayListDomain::Update(vtkSMProperty* prop)
{
  // ensures that we fire DomainModifiedEvent only once.
  DeferDomainModifiedEvents defer(this);

  vtkSMProperty* input = this->GetRequiredProperty("InputProp");
  std::string inputName = vtkSMPropertyHelper(input).GetAsString();

  vtkSMProxy* parentProxy = prop->GetParent();
  vtkSMRenderViewExporterProxy* exporterProxy =
    vtkSMRenderViewExporterProxy::SafeDownCast(parentProxy);
  if (!exporterProxy)
  {
    // error
    return;
  }

  vtkSMViewProxy* activeView = exporterProxy->GetView();
  vtkSMRenderViewProxy* rv = vtkSMRenderViewProxy::SafeDownCast(activeView);

  std::vector<std::string> arrayNames;
  vtkSMPropertyHelper helper(activeView, "Representations");
  std::map<vtkDataObject*, std::string> objectNames;
  for (unsigned int cc = 0, max = helper.GetNumberOfElements(); cc < max; ++cc)
  {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(helper.GetAsProxy(cc));

    if (!repr)
    {
      continue;
    }

    vtkSMPropertyHelper inputHelper(repr, "Input");
    vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
    std::string reprInputName = input->GetLogName();

    vtkCompositeRepresentation* compInstance =
      vtkCompositeRepresentation::SafeDownCast(repr->GetClientSideObject());
    if (!compInstance || !compInstance->GetVisibility() || reprInputName != inputName)
    {
      continue;
    }

    vtkPVDataInformation* info = input->GetDataInformation(0);
    vtkPVDataSetAttributesInformation* attrInfo = info->GetAttributeInformation(this->ArrayType);
    if (!attrInfo)
    {
      continue;
    }

    for (int i = 0; i < attrInfo->GetNumberOfArrays(); i++)
    {
      // vtkWarningMacro(<< attrInfo->GetArrayInformation(i)->GetName());
      arrayNames.emplace_back(attrInfo->GetArrayInformation(i)->GetName());
    }

    break;
  }

  // arrayNames.emplace_back("test1");
  // arrayNames.emplace_back("test321");

  this->SetStrings(arrayNames);
  this->DomainModified();
}

//---------------------------------------------------------------------------
int vtkSMPropArrayListDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }

  const char* arrayName = element->GetAttribute("array_type");
  if (!arrayName)
  {
    vtkErrorMacro("Missing array_type attribute");
    return 0;
  }
  if (arrayName)
  {
    if (!strcmp(arrayName, "point"))
    {
      this->ArrayType = vtkDataObject::AttributeTypes::POINT;
    }
    if (!strcmp(arrayName, "cell"))
    {
      this->ArrayType = vtkDataObject::AttributeTypes::CELL;
    }
  }

  vtkWarningMacro("ArrayType is " << this->ArrayType);
  return 1;
}

//---------------------------------------------------------------------------
int vtkSMPropArrayListDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values)
{
  // TODO
  return this->Superclass::SetDefaultValues(prop, use_unchecked_values);
}

//---------------------------------------------------------------------------
void vtkSMPropArrayListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
