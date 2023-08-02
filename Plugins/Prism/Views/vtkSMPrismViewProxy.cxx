// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMPrismViewProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <cassert>

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMPrismViewProxy);

//------------------------------------------------------------------------------
vtkSMPrismViewProxy::vtkSMPrismViewProxy() = default;

//------------------------------------------------------------------------------
vtkSMPrismViewProxy::~vtkSMPrismViewProxy() = default;

//------------------------------------------------------------------------------
void vtkSMPrismViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkSMPrismViewProxy::Update()
{
  this->NeedsUpdateLOD |= this->NeedsUpdate;
  bool updateWillBeCalled = this->ObjectsCreated && this->NeedsUpdate;
  this->vtkSMViewProxy::Update();
  if (updateWillBeCalled)
  {
    auto xAxisNameProp = vtkSMStringVectorProperty::SafeDownCast(this->GetProperty("XAxisName"));
    auto yAxisNameProp = vtkSMStringVectorProperty::SafeDownCast(this->GetProperty("YAxisName"));
    auto zAxisNameProp = vtkSMStringVectorProperty::SafeDownCast(this->GetProperty("ZAxisName"));
    this->UpdatePropertyInformation(xAxisNameProp);
    this->UpdatePropertyInformation(yAxisNameProp);
    this->UpdatePropertyInformation(zAxisNameProp);
    auto axesGrid = vtkSMProxyProperty::SafeDownCast(this->GetProperty("AxesGrid"))->GetProxy(0);
    vtkSMPropertyHelper(axesGrid, "XTitle").Set(xAxisNameProp->GetElement(0));
    vtkSMPropertyHelper(axesGrid, "YTitle").Set(yAxisNameProp->GetElement(0));
    vtkSMPropertyHelper(axesGrid, "ZTitle").Set(zAxisNameProp->GetElement(0));
  }
}

//----------------------------------------------------------------------------
const char* vtkSMPrismViewProxy::GetRepresentationType(vtkSMSourceProxy* producer, int outputPort)
{
  assert(producer);

  if (const char* reprName = this->vtkSMViewProxy::GetRepresentationType(producer, outputPort))
  {
    return reprName;
  }

  if (vtkPVXMLElement* hints = producer->GetHints())
  {
    // If the source has an hint as follows, then it's a text producer and must
    // be display-able.
    //  <Hints>
    //    <OutputPort name="..." index="..." type="text" />
    //  </Hints>
    for (unsigned int cc = 0, max = hints->GetNumberOfNestedElements(); cc < max; cc++)
    {
      int index;
      vtkPVXMLElement* child = hints->GetNestedElement(cc);
      const char* childName = child->GetName();
      const char* childType = child->GetAttribute("type");
      if (childName && strcmp(childName, "OutputPort") == 0 &&
        child->GetScalarAttribute("index", &index) && index == outputPort && childType)
      {
        if (strcmp(childType, "text") == 0)
        {
          return "TextSourceRepresentation";
        }
        else if (strcmp(childType, "progress") == 0)
        {
          return "ProgressBarSourceRepresentation";
        }
        else if (strcmp(childType, "logo") == 0)
        {
          return "LogoSourceRepresentation";
        }
      }
    }
  }

  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();

  const char* representationsToTry[] = { "PrismUnstructuredGridRepresentation",
    "PrismStructuredGridRepresentation", "PrismUniformGridRepresentation",
    "PrismGeometryRepresentation", nullptr };
  for (int cc = 0; representationsToTry[cc] != nullptr; ++cc)
  {
    if (vtkSMProxy* prototype = pxm->GetPrototypeProxy("representations", representationsToTry[cc]))
    {
      vtkSMProperty* inputProp = prototype->GetProperty("Input");
      vtkSMUncheckedPropertyHelper helper(inputProp);
      helper.Set(producer, outputPort);
      bool acceptable = (inputProp->IsInDomains() > 0);
      helper.SetNumberOfElements(0);
      if (acceptable)
      {
        return representationsToTry[cc];
      }
    }
  }

  {
    vtkPVDataInformation* dataInformation = nullptr;
    if (vtkSMOutputPort* port = producer->GetOutputPort(outputPort))
    {
      dataInformation = port->GetDataInformation();
    }

    // check if the data type is a vtkTable with a single row and column with
    // a vtkStringArray named "Text". If it is, we render this in a render view
    // with the value shown in the view.
    if (dataInformation)
    {
      if (dataInformation->GetDataSetType() == VTK_TABLE)
      {
        if (vtkPVArrayInformation* ai =
              dataInformation->GetArrayInformation("Text", vtkDataObject::ROW))
        {
          if (ai->GetNumberOfComponents() == 1 && ai->GetNumberOfTuples() == 1)
          {
            return "TextSourceRepresentation";
          }
        }
      }
    }
  }

  // Default to "PrismGeometryRepresentation" for composite datasets where we
  // might not yet know the dataset type of the children at the time the
  // representation is created.
  if (vtkSMOutputPort* port = producer->GetOutputPort(outputPort))
  {
    if (vtkPVDataInformation* dataInformation = port->GetDataInformation())
    {
      if (dataInformation->IsCompositeDataSet())
      {
        return "PrismGeometryRepresentation";
      }
    }
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkSMPrismViewProxy::CopySelectionRepresentationProperties(
  vtkSMProxy* fromSelectionRep, vtkSMProxy* toSelectionRep)
{
  if (!fromSelectionRep || !toSelectionRep)
  {
    return;
  }
  if (strcmp(fromSelectionRep->GetXMLName(), this->GetSelectionRepresentationProxyName()) == 0 &&
    strcmp(toSelectionRep->GetXMLName(), this->GetSelectionRepresentationProxyName()) == 0)
  {
    toSelectionRep->GetProperty("IsSimulationData")
      ->Copy(fromSelectionRep->GetProperty("IsSimulationData"));
    toSelectionRep->GetProperty("AttributeType")
      ->Copy(fromSelectionRep->GetProperty("AttributeType"));
    toSelectionRep->GetProperty("XArrayName")->Copy(fromSelectionRep->GetProperty("XArrayName"));
    toSelectionRep->GetProperty("YArrayName")->Copy(fromSelectionRep->GetProperty("YArrayName"));
    toSelectionRep->GetProperty("ZArrayName")->Copy(fromSelectionRep->GetProperty("ZArrayName"));
    toSelectionRep->UpdateVTKObjects();
  }
}
