// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMPVRepresentationProxy.h"

#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVProminentValuesInformation.h"
#include "vtkPVTemporalDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtksys/SystemTools.hxx"

#include <string>

vtkCxxSetSmartPointerMacro(vtkSMPVRepresentationProxy, LastLookupTable, vtkSMProxy);

vtkStandardNewMacro(vtkSMPVRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::vtkSMPVRepresentationProxy()
{
  this->SetSIClassName("vtkSIPVRepresentationProxy");
  this->InReadXMLAttributes = false;
  this->LastLookupTable = nullptr;
}

//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::~vtkSMPVRepresentationProxy() = default;

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMPVRepresentationProxy::GetLastLookupTable()
{
  return this->LastLookupTable;
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetLastBlocksLookupTable(
  const std::vector<std::string>& blockSelectors, vtkSMProxy* proxy)
{
  for (const auto& selector : blockSelectors)
  {
    this->LastBlocksLookupTables[selector] = proxy;
  }
}

//----------------------------------------------------------------------------
std::vector<vtkSMProxy*> vtkSMPVRepresentationProxy::GetLastBlocksLookupTables(
  const std::vector<std::string>& blockSelectors)
{
  std::vector<vtkSMProxy*> proxies;
  for (const auto& selector : blockSelectors)
  {
    auto found = this->LastBlocksLookupTables.find(selector);
    if (found == this->LastBlocksLookupTables.end())
    {
      proxies.push_back(nullptr);
    }
    else
    {
      proxies.push_back(found->second);
    }
  }
  return proxies;
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }
  this->Superclass::CreateVTKObjects();
  if (!this->ObjectsCreated)
  {
    return;
  }

  // Ensure that we update the RepresentationTypesInfo property and the domain
  // for "Representations" property before CreateVTKObjects() is finished. This
  // ensure that all representations have valid Representations domain.
  this->UpdatePropertyInformation();

  // Whenever the "Representation" property is modified, we ensure that the
  // this->InvalidateDataInformation() is called.
  this->AddObserver(
    vtkCommand::UpdatePropertyEvent, this, &vtkSMPVRepresentationProxy::OnPropertyUpdated);
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::OnPropertyUpdated(vtkObject*, unsigned long, void* calldata)
{
  const char* pname = reinterpret_cast<const char*>(calldata);
  if (pname && strcmp(pname, "Representation") == 0)
  {
    this->InvalidateDataInformation();
  }
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetPropertyModifiedFlag(const char* name, int flag)
{
  if (!this->InReadXMLAttributes && name && strcmp(name, "Input") == 0)
  {
    // Whenever the input for the representation is set, we need to setup the
    // the input for the internal selection representation that shows the
    // extracted-selection. This is done at the proxy level so that whenever the
    // selection is changed in the application, the SelectionRepresentation is
    // 'MarkedModified' correctly, so that it updates itself cleanly.
    vtkSMProxy* selectionRepr = this->GetSubProxy("SelectionRepresentation");
    const vtkSMPropertyHelper helper(this, name);
    for (unsigned int cc = 0; cc < helper.GetNumberOfElements(); cc++)
    {
      vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(helper.GetAsProxy(cc));
      if (input && selectionRepr)
      {
        input->CreateSelectionProxies();
        vtkSMSourceProxy* esProxy = input->GetSelectionOutput(helper.GetOutputPort(cc));
        if (!esProxy)
        {
          vtkErrorMacro("Input proxy does not support selection extraction.");
        }
        else
        {
          int port = 0;
          if (vtkPVXMLElement* hints = selectionRepr->GetHints()
              ? selectionRepr->GetHints()->FindNestedElementByName("ConnectToPortIndex")
              : nullptr)
          {
            hints->GetScalarAttribute("value", &port);
          }

          vtkSMPropertyHelper(selectionRepr, "Input").Set(esProxy, port);
          selectionRepr->UpdateVTKObjects();
        }
      }
    }
  }

  this->Superclass::SetPropertyModifiedFlag(name, flag);
}

//----------------------------------------------------------------------------
int vtkSMPVRepresentationProxy::ReadXMLAttributes(
  vtkSMSessionProxyManager* pm, vtkPVXMLElement* element)
{
  this->InReadXMLAttributes = true;
  for (unsigned int cc = 0; cc < element->GetNumberOfNestedElements(); ++cc)
  {
    vtkPVXMLElement* child = element->GetNestedElement(cc);
    if (child->GetName() && strcmp(child->GetName(), "RepresentationType") == 0 &&
      child->GetAttribute("subproxy") != nullptr)
    {
      this->RepresentationSubProxies.insert(child->GetAttribute("subproxy"));
    }
  }

  const int retVal = this->Superclass::ReadXMLAttributes(pm, element);
  this->InReadXMLAttributes = false;

  // Setup property links for sub-proxies. This ensures that whenever the
  // this->GetProperty("Input") changes (either checked or un-checked values),
  // all the sub-proxy's "Input" is also changed to the same value. This ensures
  // that the domains are updated correctly.
  vtkSMProperty* inputProperty = this->GetProperty("Input");
  if (inputProperty)
  {
    for (auto& subProxyName : this->RepresentationSubProxies)
    {
      vtkSMProxy* subProxy = this->GetSubProxy(subProxyName.c_str());
      vtkSMProperty* subProperty = subProxy ? subProxy->GetProperty("Input") : nullptr;
      if (subProperty)
      {
        this->LinkProperty(inputProperty, subProperty);
      }
    }
  }
  return retVal;
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::GetUsingScalarColoring()
{
  return vtkSMColorMapEditorHelper::GetUsingScalarColoring(this);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMPVRepresentationProxy::GetBlocksUsingScalarColoring(
  const std::vector<std::string>& blockSelector)
{
  return vtkSMColorMapEditorHelper::GetBlocksUsingScalarColoring(this, blockSelector);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::GetAnyBlockUsingScalarColoring()
{
  return vtkSMColorMapEditorHelper::GetAnyBlockUsingScalarColoring(this);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(bool extend, bool force)
{
  return vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(this, extend, force);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMPVRepresentationProxy::RescaleBlocksTransferFunctionToDataRange(
  const std::vector<std::string>& blockSelectors, bool extend, bool force)
{
  return vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRange(
    this, blockSelectors, extend, force);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(
  const char* arrayName, int attributeType, bool extend, bool force)
{
  return vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(
    this, arrayName, attributeType, extend, force);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMPVRepresentationProxy::RescaleBlocksTransferFunctionToDataRange(
  const std::vector<std::string>& blockSelectors, const char* arrayName, int attributeType,
  bool extend, bool force)
{
  return vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRange(
    this, blockSelectors, arrayName, attributeType, extend, force);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRangeOverTime()
{
  return vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRangeOverTime(this);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool>
vtkSMPVRepresentationProxy::RescaleBlocksTransferFunctionToDataRangeOverTime(
  const std::vector<std::string>& blockSelectors)
{
  return vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRangeOverTime(
    this, blockSelectors);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRangeOverTime(
  const char* arrayName, int attributeType)
{
  return vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRangeOverTime(
    this, arrayName, attributeType);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool>
vtkSMPVRepresentationProxy::RescaleBlocksTransferFunctionToDataRangeOverTime(
  const std::vector<std::string>& blockSelectors, const char* arrayName, int attributeType)
{
  return vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRangeOverTime(
    this, blockSelectors, arrayName, attributeType);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToVisibleRange(vtkSMProxy* view)
{
  return vtkSMColorMapEditorHelper::RescaleTransferFunctionToVisibleRange(this, view);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToVisibleRange(
  vtkSMProxy* view, const char* arrayName, int attributeType)
{
  return vtkSMColorMapEditorHelper::RescaleTransferFunctionToVisibleRange(
    this, view, arrayName, attributeType);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::SetScalarColoring(const char* arrayName, int attributeType)
{
  return vtkSMColorMapEditorHelper::SetScalarColoring(this, arrayName, attributeType);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMPVRepresentationProxy::SetBlocksScalarColoring(
  const std::vector<std::string>& blockSelectors, const char* arrayName, int attributeType)
{
  return vtkSMColorMapEditorHelper::SetBlocksScalarColoring(
    this, blockSelectors, arrayName, attributeType);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::SetScalarColoring(
  const char* arrayName, int attributeType, int component)
{
  return vtkSMColorMapEditorHelper::SetScalarColoring(this, arrayName, attributeType, component);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMPVRepresentationProxy::SetBlocksScalarColoring(
  const std::vector<std::string>& blockSelectors, const char* arrayName, int attributeType,
  int component)
{
  return vtkSMColorMapEditorHelper::SetBlocksScalarColoring(
    this, blockSelectors, arrayName, attributeType, component);
}

//----------------------------------------------------------------------------
int vtkSMPVRepresentationProxy::IsScalarBarStickyVisible(vtkSMProxy* view)
{
  return vtkSMColorMapEditorHelper::IsScalarBarStickyVisible(this, view);
}

//----------------------------------------------------------------------------
std::vector<int> vtkSMPVRepresentationProxy::IsBlocksScalarBarStickyVisible(
  vtkSMProxy* view, const std::vector<std::string>& blockSelectors)
{
  return vtkSMColorMapEditorHelper::IsBlocksScalarBarStickyVisible(this, view, blockSelectors);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::UpdateScalarBarRange(vtkSMProxy* view, bool deleteRange)
{
  return vtkSMColorMapEditorHelper::UpdateScalarBarRange(this, view, deleteRange);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMPVRepresentationProxy::UpdateBlocksScalarBarRange(
  vtkSMProxy* view, bool deleteRange)
{
  return vtkSMColorMapEditorHelper::UpdateBlocksScalarBarRange(this, view, deleteRange);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMPVRepresentationProxy::GetLookupTable(vtkSMProxy* view)
{
  return vtkSMColorMapEditorHelper::GetLookupTable(this, view);
}

//----------------------------------------------------------------------------
std::vector<vtkSMProxy*> vtkSMPVRepresentationProxy::GetBlocksLookupTables(
  vtkSMProxy* view, const std::vector<std::string>& blockSelectors)
{
  return vtkSMColorMapEditorHelper::GetBlocksLookupTables(this, view, blockSelectors);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::SetScalarBarVisibility(vtkSMProxy* view, bool visible)
{
  return vtkSMColorMapEditorHelper::SetScalarBarVisibility(this, view, visible);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMPVRepresentationProxy::SetBlocksScalarBarVisibility(
  vtkSMProxy* view, const std::vector<std::string>& blockSelectors, bool visible)
{
  return vtkSMColorMapEditorHelper::SetBlocksScalarBarVisibility(
    this, view, blockSelectors, visible);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::HideScalarBarIfNotNeeded(vtkSMProxy* view)
{
  return vtkSMColorMapEditorHelper::HideScalarBarIfNotNeeded(this, view);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::HideBlocksScalarBarIfNotNeeded(vtkSMProxy* view)
{
  return vtkSMColorMapEditorHelper::HideBlocksScalarBarIfNotNeeded(this, view);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::IsScalarBarVisible(vtkSMProxy* view)
{
  return vtkSMColorMapEditorHelper::IsScalarBarVisible(this, view);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMPVRepresentationProxy::IsBlocksScalarBarVisible(
  vtkSMProxy* view, const std::vector<std::string>& blockSelectors)
{
  return vtkSMColorMapEditorHelper::IsBlocksScalarBarVisible(this, view, blockSelectors);
}

//----------------------------------------------------------------------------
vtkPVArrayInformation* vtkSMPVRepresentationProxy::GetArrayInformationForColorArray(
  bool checkRepresentedData)
{
  return vtkSMColorMapEditorHelper::GetArrayInformationForColorArray(this, checkRepresentedData);
}

//----------------------------------------------------------------------------
std::vector<vtkPVArrayInformation*>
vtkSMPVRepresentationProxy::GetBlocksArrayInformationForColorArray(
  const std::vector<std::string>& blockSelectors)
{
  return vtkSMColorMapEditorHelper::GetBlocksArrayInformationForColorArray(this, blockSelectors);
}

//----------------------------------------------------------------------------
vtkPVProminentValuesInformation*
vtkSMPVRepresentationProxy::GetProminentValuesInformationForColorArray(
  double uncertaintyAllowed, double fraction, bool force)
{
  return vtkSMColorMapEditorHelper::GetProminentValuesInformationForColorArray(
    this, uncertaintyAllowed, fraction, force);
}

//----------------------------------------------------------------------------
std::vector<vtkPVProminentValuesInformation*>
vtkSMPVRepresentationProxy::GetBlocksProminentValuesInformationForColorArray(
  const std::vector<std::string>& blockSelectors, double uncertaintyAllowed, double fraction,
  bool force)
{
  return vtkSMColorMapEditorHelper::GetBlocksProminentValuesInformationForColorArray(
    this, blockSelectors, uncertaintyAllowed, fraction, force);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::SetRepresentationType(const char* type)
{
#define CALL_SUPERCLASS 0
  try
  {
    if (type == nullptr)
    {
      throw CALL_SUPERCLASS;
    }

    if (strcmp(type, "Volume") != 0 && strcmp(type, "Slice") != 0 &&
      vtksys::SystemTools::Strucmp(type, "nvidia index") != 0)
    {
      throw CALL_SUPERCLASS;
    }

    // we are changing to volume or slice representation.
    if (this->GetUsingScalarColoring())
    {
      throw CALL_SUPERCLASS;
    }

    // pick a color array and then accept or fail as applicable.
    vtkSMProperty* colorArrayName = this->GetProperty("ColorArrayName");
    if (colorArrayName)
    {
      auto ald = colorArrayName->FindDomain<vtkSMArrayListDomain>();
      if (ald && ald->GetNumberOfStrings() > 0)
      {
        unsigned int index = 0;
        // if possible, pick a "point" array since that works better with some
        // crappy volume renderers. We need to fixed all volume mapper to not
        // segfault when cell data is picked.
        for (unsigned int cc = 0, max = ald->GetNumberOfStrings(); cc < max; cc++)
        {
          if (ald->GetFieldAssociation(cc) == vtkDataObject::POINT)
          {
            index = cc;
            break;
          }
        }
        if (this->SetScalarColoring(ald->GetString(index), ald->GetFieldAssociation(index)))
        {
          // Ensure that the transfer function is rescaled, as if user picked the array to color
          // with from the UI. I wonder if SetScalarColoring should really take care of it.
          this->RescaleTransferFunctionToDataRange(true);
          throw CALL_SUPERCLASS;
        }
      }
    }
  }
  catch (const int& val)
  {
    if (val == CALL_SUPERCLASS)
    {
      return this->Superclass::SetRepresentationType(type);
    }
  }
  // It's not sure if the we should error out or still do the change. Opting for
  // going further with the change in representation type for now.
  return this->Superclass::SetRepresentationType(type);
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkSMPVRepresentationProxy::GetEstimatedNumberOfAnnotationsOnScalarBar(vtkSMProxy* view)
{
  return vtkSMColorMapEditorHelper::GetEstimatedNumberOfAnnotationsOnScalarBar(this, view);
}

//----------------------------------------------------------------------------
std::vector<int> vtkSMPVRepresentationProxy::GetBlocksEstimatedNumberOfAnnotationsOnScalarBars(
  vtkSMProxy* view, const std::vector<std::string>& blockSelectors)
{
  return vtkSMColorMapEditorHelper::GetBlocksEstimatedNumberOfAnnotationsOnScalarBars(
    this, view, blockSelectors);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::GetVolumeIndependentRanges()
{
  // the representation is Volume
  vtkSMProperty* repProperty = this->GetProperty("Representation");
  if (!repProperty || strcmp(vtkSMPropertyHelper(repProperty).GetAsString(), "Volume") != 0)
  {
    return false;
  }

  // MapScalars and (MultiComponentsMapping or UseSeparateOpacityArray) are checked
  vtkSMProperty* msProperty = this->GetProperty("MapScalars");
  vtkSMProperty* mcmProperty = this->GetProperty("MultiComponentsMapping");
  vtkSMProperty* uoaProperty = this->GetProperty("UseSeparateOpacityArray");
  return (vtkSMPropertyHelper(msProperty).GetAsInt() != 0 &&
    (vtkSMPropertyHelper(mcmProperty).GetAsInt() != 0 ||
      vtkSMPropertyHelper(uoaProperty).GetAsInt() != 0));
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::ViewUpdated(vtkSMProxy* view)
{
  if (this->GetProperty("Visibility"))
  {
    const bool visible = vtkSMPropertyHelper(this, "Visibility").GetAsInt() != 0;
    if (visible && this->GetUsingScalarColoring())
    {
      this->UpdateScalarBarRange(view, false /* deleteRange */);
    }
    else
    {
      this->UpdateScalarBarRange(view, true /* deleteRange */);
    }
    if (visible && this->GetAnyBlockUsingScalarColoring())
    {
      this->UpdateBlocksScalarBarRange(view, false /* deleteRange */);
    }
    else
    {
      this->UpdateBlocksScalarBarRange(view, true /* deleteRange */);
    }
  }
  this->Superclass::ViewUpdated(view);
}

//----------------------------------------------------------------------------
std::vector<std::pair<int, std::string>> vtkSMPVRepresentationProxy::GetBlocksColorArrays(
  const std::vector<std::string>& blockSelectors)
{
  return vtkSMColorMapEditorHelper::GetBlocksColorArrays(this, blockSelectors);
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetBlocksUseSeparateColorMap(
  const std::vector<std::string>& blockSelectors, bool use)
{
  vtkSMColorMapEditorHelper::SetBlocksUseSeparateColorMap(this, blockSelectors, use);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMPVRepresentationProxy::GetBlocksUseSeparateColorMaps(
  const std::vector<std::string>& blockSelectors)
{
  return vtkSMColorMapEditorHelper::GetBlocksUseSeparateColorMaps(this, blockSelectors);
}
