// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMPropDomain.h"

#include "vtkCompositeRepresentation.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkPVCompositeRepresentation.h"
#include "vtkPVDataRepresentation.h"
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

vtkStandardNewMacro(vtkSMPropDomain);

//---------------------------------------------------------------------------
vtkSMPropDomain::vtkSMPropDomain()
{
  // vtkWarningMacro("Create prop domain");
}

//---------------------------------------------------------------------------
vtkSMPropDomain::~vtkSMPropDomain() {}

//---------------------------------------------------------------------------
void vtkSMPropDomain::Update(vtkSMProperty* prop)
{
  // ensures that we fire DomainModifiedEvent only once.
  DeferDomainModifiedEvents defer(this);
  std::vector<std::string> propNames;

  // vtkSMProxy* parentProxy = this->GetProperty()->GetParent();
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

    vtkCompositeRepresentation* compInstance =
      vtkCompositeRepresentation::SafeDownCast(repr->GetClientSideObject());
    if (compInstance->GetVisibility())
    {
      // Unused
      objectNames.insert({ compInstance->GetRenderedDataObject(0), input->GetLogName() });

      propNames.emplace_back(input->GetLogName());
    }
  }

  this->SetStrings(propNames);
  this->DomainModified();
}

//---------------------------------------------------------------------------
int vtkSMPropDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }

  return 1;
}

//---------------------------------------------------------------------------
int vtkSMPropDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values)
{
  // vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  // if (!svp)
  // {
  //   return 0;
  // }

  // TODO
  return this->Superclass::SetDefaultValues(prop, use_unchecked_values);
}

//---------------------------------------------------------------------------
void vtkSMPropDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
