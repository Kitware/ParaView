// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMPropArrayListDomain.h"

#include "vtkCompositeRepresentation.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewExporterProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSourceProxy.h"

#include <vector>

vtkStandardNewMacro(vtkSMPropArrayListDomain);

//---------------------------------------------------------------------------
void vtkSMPropArrayListDomain::Update(vtkSMProperty* prop)
{
  // ensures that we fire DomainModifiedEvent only once.
  vtkSMDomain::DeferDomainModifiedEvents defer(this);

  vtkSMProxy* parentProxy = prop->GetParent();
  vtkSMRenderViewExporterProxy* exporterProxy =
    vtkSMRenderViewExporterProxy::SafeDownCast(parentProxy);
  if (!exporterProxy)
  {
    vtkErrorMacro("Could not find exporter proxy");
    return;
  }

  vtkSMViewProxy* activeView = exporterProxy->GetView();

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
    if (!compInstance || !compInstance->GetVisibility())
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
      arrayNames.emplace_back(
        reprInputName + ":" + std::string(attrInfo->GetArrayInformation(i)->GetName()));
    }
  }

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

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMPropArrayListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Array Type: "
     << (this->ArrayType == vtkDataObject::AttributeTypes::POINT ? "point" : "cell");
}
