// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMPVRepresentationProxy.h"

#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVProminentValuesInformation.h"
#include "vtkPVRepresentedArrayListSettings.h"
#include "vtkPVTemporalDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSettings.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTrace.h"
#include "vtkSMTransferFunction2DProxy.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkStringList.h"
#include "vtksys/SystemTools.hxx"

#include <cmath>
#include <set>
#include <sstream>
#include <string>

class vtkSMPVRepresentationProxy::vtkStringSet : public std::set<std::string>
{
};

vtkCxxSetSmartPointerMacro(vtkSMPVRepresentationProxy, LastLUTProxy, vtkSMProxy);

vtkStandardNewMacro(vtkSMPVRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::vtkSMPVRepresentationProxy()
{
  this->SetSIClassName("vtkSIPVRepresentationProxy");
  this->RepresentationSubProxies = new vtkStringSet();
  this->InReadXMLAttributes = false;
  this->LastLUTProxy = nullptr;
}

//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::~vtkSMPVRepresentationProxy()
{
  delete this->RepresentationSubProxies;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMPVRepresentationProxy::GetLastLUTProxy()
{
  return this->LastLUTProxy;
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
    vtkSMPropertyHelper helper(this, name);
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
      this->RepresentationSubProxies->insert(child->GetAttribute("subproxy"));
    }
  }

  int retVal = this->Superclass::ReadXMLAttributes(pm, element);
  this->InReadXMLAttributes = false;

  // Setup property links for sub-proxies. This ensures that whenever the
  // this->GetProperty("Input") changes (either checked or un-checked values),
  // all the sub-proxy's "Input" is also changed to the same value. This ensures
  // that the domains are updated correctly.
  vtkSMProperty* inputProperty = this->GetProperty("Input");
  if (inputProperty)
  {
    for (vtkStringSet::iterator iter = this->RepresentationSubProxies->begin();
         iter != this->RepresentationSubProxies->end(); ++iter)
    {
      vtkSMProxy* subProxy = this->GetSubProxy((*iter).c_str());
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
void vtkSMPVRepresentationProxy::SetupLookupTable(vtkSMProxy* proxy)
{
  vtkSMColorMapEditorHelper::SetupLookupTable(proxy);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(bool extend, bool force)
{
  return vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(this, extend, force);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(
  const char* arrayname, int attribute_type, bool extend, bool force)
{
  return vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(
    this, arrayname, attribute_type, extend, force);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRangeOverTime()
{
  return vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRangeOverTime(this);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRangeOverTime(
  const char* arrayname, int attribute_type)
{
  return vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRangeOverTime(
    this, arrayname, attribute_type);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(
  vtkPVArrayInformation* info, bool extend, bool force)
{
  if (!info)
  {
    vtkWarningMacro("Could not determine array range.");
    return false;
  }

  vtkSMProperty* lutProperty = this->GetProperty("LookupTable");
  vtkSMProperty* sofProperty = this->GetProperty("ScalarOpacityFunction");
  vtkSMProperty* tf2dProperty = this->GetProperty("TransferFunction2D");
  if (!lutProperty && !sofProperty && !tf2dProperty)
  {
    vtkWarningMacro("No 'LookupTable' and 'ScalarOpacityFunction' found.");
    return false;
  }

  vtkSMProxy* lut = vtkSMPropertyHelper(lutProperty).GetAsProxy();
  vtkSMProxy* sof = vtkSMPropertyHelper(sofProperty).GetAsProxy();
  vtkSMProxy* tf2d = vtkSMPropertyHelper(tf2dProperty).GetAsProxy();

  if (force == false &&
    vtkSMPropertyHelper(lut, "AutomaticRescaleRangeMode", true).GetAsInt() ==
      vtkSMTransferFunctionManager::NEVER)
  {
    // nothing to change, range is locked.
    return true;
  }

  // We need to determine the component number to use from the lut.
  int component = -1;
  if (lut && vtkSMPropertyHelper(lut, "VectorMode").GetAsInt() != 0)
  {
    component = vtkSMPropertyHelper(lut, "VectorComponent").GetAsInt();
  }

  if (lut && component < info->GetNumberOfComponents())
  {
    int indexedLookup = vtkSMPropertyHelper(lut, "IndexedLookup").GetAsInt();
    if (indexedLookup)
    {
      vtkPVProminentValuesInformation* prominentValues =
        vtkSMColorMapEditorHelper::GetProminentValuesInformationForColorArray(this);
      vtkSmartPointer<vtkStringList> activeAnnotations = vtkSmartPointer<vtkStringList>::New();
      vtkSmartPointer<vtkDoubleArray> activeIndexedColors = vtkSmartPointer<vtkDoubleArray>::New();
      vtkSmartPointer<vtkAbstractArray> uniqueValues;

      uniqueValues.TakeReference(prominentValues->GetProminentComponentValues(component));

      vtkSMStringVectorProperty* allAnnotations =
        vtkSMStringVectorProperty::SafeDownCast(lut->GetProperty("Annotations"));
      vtkSMStringVectorProperty* activeAnnotatedValuesProperty =
        vtkSMStringVectorProperty::SafeDownCast(lut->GetProperty("ActiveAnnotatedValues"));
      if (uniqueValues && allAnnotations && activeAnnotatedValuesProperty)
      {
        vtkSmartPointer<vtkStringList> activeAnnotatedValues =
          vtkSmartPointer<vtkStringList>::New();

        if (extend)
        {
          activeAnnotatedValuesProperty->GetElements(activeAnnotatedValues);
        }

        for (int idx = 0; idx < uniqueValues->GetNumberOfTuples(); ++idx)
        {
          // Look up index of color corresponding to the annotation
          for (unsigned int j = 0; j < allAnnotations->GetNumberOfElements() / 2; ++j)
          {
            vtkVariant annotatedValue(allAnnotations->GetElement(2 * j + 0));
            if (annotatedValue == uniqueValues->GetVariantValue(idx))
            {
              activeAnnotatedValues->AddString(allAnnotations->GetElement(2 * j + 0));
              break;
            }
          }
        }

        activeAnnotatedValuesProperty->SetElements(activeAnnotatedValues);
        lut->UpdateVTKObjects();
      }
    }
    else
    {
      double rangeColor[2];
      double rangeOpacity[2];

      bool useOpacityArray = false;
      if (auto uoaProperty = this->GetProperty("UseSeparateOpacityArray"))
      {
        useOpacityArray = vtkSMPropertyHelper(uoaProperty).GetAsInt() == 1;
      }

      if (useOpacityArray)
      {
        vtkSMPropertyHelper inputHelper(this->GetProperty("Input"));
        vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
        int port = inputHelper.GetOutputPort();
        if (!inputProxy)
        {
          // no input.
          vtkWarningMacro("No input present. Cannot determine opacity data range.");
          return false;
        }

        vtkPVDataInformation* dataInfo = inputProxy->GetDataInformation(port);
        vtkSMPropertyHelper opacityArrayNameHelper(this, "OpacityArrayName");
        int opacityArrayFieldAssociation = opacityArrayNameHelper.GetAsInt(3);
        const char* opacityArrayName = opacityArrayNameHelper.GetAsString(4);
        int opacityArrayComponent = vtkSMPropertyHelper(this, "OpacityComponent").GetAsInt();

        vtkPVArrayInformation* opacityArrayInfo =
          dataInfo->GetArrayInformation(opacityArrayName, opacityArrayFieldAssociation);
        if (opacityArrayComponent >= opacityArrayInfo->GetNumberOfComponents())
        {
          opacityArrayComponent = -1;
        }
        info->GetComponentFiniteRange(component, rangeColor);
        opacityArrayInfo->GetComponentFiniteRange(opacityArrayComponent, rangeOpacity);
      }
      else if (this->GetVolumeIndependentRanges())
      {
        info->GetComponentFiniteRange(0, rangeColor);
        info->GetComponentFiniteRange(1, rangeOpacity);
      }
      else
      {
        info->GetComponentFiniteRange(component, rangeColor);
        rangeOpacity[0] = rangeColor[0];
        rangeOpacity[1] = rangeColor[1];
      }

      // the range must be large enough, compared to values order of magnitude
      // If data range is too small then we tweak it a bit so scalar mapping
      // produces valid/reproducible results.
      vtkSMCoreUtilities::AdjustRange(rangeColor);
      vtkSMCoreUtilities::AdjustRange(rangeOpacity);

      if (lut && rangeColor[1] >= rangeColor[0])
      {
        vtkSMTransferFunctionProxy::RescaleTransferFunction(lut, rangeColor, extend);
        vtkSMProxy* sof_lut = vtkSMPropertyHelper(lut, "ScalarOpacityFunction", true).GetAsProxy();
        if (sof_lut && sof != sof_lut && rangeOpacity[1] >= rangeOpacity[0])
        {
          vtkSMTransferFunctionProxy::RescaleTransferFunction(sof_lut, rangeOpacity, extend);
        }
      }

      if (sof && rangeOpacity[1] >= rangeOpacity[0])
      {
        vtkSMTransferFunctionProxy::RescaleTransferFunction(sof, rangeOpacity, extend);
      }

      if (tf2d && rangeColor[1] >= rangeColor[0])
      {
        double range2D[4];
        vtkSMTransferFunction2DProxy::GetRange(tf2d, range2D);
        range2D[0] = rangeColor[0];
        range2D[1] = rangeColor[1];
        vtkSMTransferFunction2DProxy::RescaleTransferFunction(tf2d, range2D);
      }
      return (lut || sof || tf2d);
    }
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToVisibleRange(vtkSMProxy* view)
{
  return vtkSMColorMapEditorHelper::RescaleTransferFunctionToVisibleRange(this, view);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToVisibleRange(
  vtkSMProxy* view, const char* arrayname, int attribute_type)
{
  return vtkSMColorMapEditorHelper::RescaleTransferFunctionToVisibleRange(
    this, view, arrayname, attribute_type);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::SetScalarColoring(const char* arrayname, int attribute_type)
{
  return vtkSMColorMapEditorHelper::SetScalarColoring(this, arrayname, attribute_type);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::SetScalarColoring(
  const char* arrayname, int attribute_type, int component)
{
  return vtkSMColorMapEditorHelper::SetScalarColoring(this, arrayname, attribute_type, component);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::SetScalarColoringInternal(
  const char* arrayname, int attribute_type, bool useComponent, int component)
{
  if (!this->GetUsingScalarColoring() && (arrayname == nullptr || arrayname[0] == 0))
  {
    // scalar coloring already off. Nothing to do.
    return true;
  }

  vtkSMProperty* colorArray = this->GetProperty("ColorArrayName");
  if (!colorArray)
  {
    vtkWarningMacro("No 'ColorArrayName' property found.");
    return false;
  }

  vtkSMPropertyHelper colorArrayHelper(colorArray);
  colorArrayHelper.SetInputArrayToProcess(attribute_type, arrayname);

  if (arrayname == nullptr || arrayname[0] == '\0')
  {
    SM_SCOPED_TRACE(SetScalarColoring)
      .arg("display", this)
      .arg("arrayname", arrayname)
      .arg("attribute_type", attribute_type);
    vtkSMPropertyHelper(this, "LookupTable", true).RemoveAllValues();
    vtkSMPropertyHelper(this, "ScalarOpacityFunction", true).RemoveAllValues();
    this->UpdateVTKObjects();
    return true;
  }

  auto* arraySettings = vtkPVRepresentedArrayListSettings::GetInstance();
  auto* arrayInfo = this->GetArrayInformationForColorArray(false);
  const bool forceComponentMode = (arrayInfo && arraySettings &&
    !arraySettings->ShouldUseMagnitudeMode(arrayInfo->GetNumberOfComponents()));
  if (forceComponentMode && (!useComponent || component < 0))
  {
    component = 0;
  }

  // Now, setup transfer functions.
  bool haveComponent = useComponent;
  bool separate = (vtkSMPropertyHelper(this, "UseSeparateColorMap", true).GetAsInt() != 0);
  bool useTransfer2D = (vtkSMPropertyHelper(this, "UseTransfer2D", true).GetAsInt() != 0);
  std::string decoratedArrayName =
    vtkSMColorMapEditorHelper::GetDecoratedArrayName(this, arrayname);
  vtkNew<vtkSMTransferFunctionManager> mgr;
  vtkSMProxy* lutProxy = nullptr;
  if (vtkSMProperty* lutProperty = this->GetProperty("LookupTable"))
  {
    lutProxy =
      mgr->GetColorTransferFunction(decoratedArrayName.c_str(), this->GetSessionProxyManager());
    if (useComponent || forceComponentMode)
    {
      if (component >= 0)
      {
        vtkSMPropertyHelper(lutProxy, "VectorMode").Set("Component");
        vtkSMPropertyHelper(lutProxy, "VectorComponent").Set(component);
        lutProxy->UpdateVTKObjects();
      }
      else
      {
        vtkSMPropertyHelper(lutProxy, "VectorMode").Set("Magnitude");
        lutProxy->UpdateVTKObjects();
      }
    }
    else
    {
      // No Component defined for coloring, in order to generate a valid trace
      // a component is needed, recover currently used component
      const char* vectorMode = vtkSMPropertyHelper(lutProxy, "VectorMode").GetAsString();
      haveComponent = true;
      if (strcmp(vectorMode, "Component") == 0)
      {
        component = vtkSMPropertyHelper(lutProxy, "VectorComponent").GetAsInt();
      }
      else // Magnitude
      {
        component = -1;
      }
    }

    vtkSMPropertyHelper(lutProperty).Set(lutProxy);

    // Get the array information for the color array to determine transfer function properties
    vtkPVArrayInformation* colorArrayInfo = this->GetArrayInformationForColorArray();
    if (colorArrayInfo)
    {
      if (colorArrayInfo->GetDataType() == VTK_STRING)
      {
        vtkSMPropertyHelper(lutProxy, "IndexedLookup", true).Set(1);
        lutProxy->UpdateVTKObjects();
      }
      if (haveComponent)
      {
        const char* componentName = colorArrayInfo->GetComponentName(component);
        if (strcmp(componentName, "") != 0)
        {
          SM_SCOPED_TRACE(SetScalarColoring)
            .arg("display", this)
            .arg("arrayname", arrayname)
            .arg("attribute_type", attribute_type)
            .arg("component", componentName)
            .arg("separate", separate);
        }
        else
        {
          haveComponent = false;
        }
      }
    }
  }

  if (!haveComponent)
  {
    SM_SCOPED_TRACE(SetScalarColoring)
      .arg("display", this)
      .arg("arrayname", arrayname)
      .arg("attribute_type", attribute_type)
      .arg("separate", separate);
  }

  if (vtkSMProperty* sofProperty = this->GetProperty("ScalarOpacityFunction"))
  {
    vtkSMProxy* sofProxy =
      mgr->GetOpacityTransferFunction(decoratedArrayName.c_str(), this->GetSessionProxyManager());
    vtkSMPropertyHelper(sofProperty).Set(sofProxy);
  }

  if (vtkSMProperty* tf2dProperty = this->GetProperty("TransferFunction2D"))
  {
    vtkSMProxy* tf2dProxy =
      mgr->GetTransferFunction2D(decoratedArrayName.c_str(), this->GetSessionProxyManager());
    vtkSMPropertyHelper(tf2dProperty).Set(tf2dProxy);
    this->UpdateProperty("TransferFunction2D");
    if (lutProxy && useTransfer2D)
    {
      vtkSMPropertyHelper(lutProxy, "Using2DTransferFunction").Set(useTransfer2D);
      lutProxy->UpdateVTKObjects();
    }
  }

  this->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
std::string vtkSMPVRepresentationProxy::GetDecoratedArrayName(const std::string& arrayname)
{
  std::ostringstream ss;
  ss << arrayname;
  if (this->GetProperty("TransferFunction2D"))
  {
    bool useGradientAsY =
      (vtkSMPropertyHelper(this, "UseGradientForTransfer2D", true).GetAsInt() == 1);
    if (!useGradientAsY)
    {
      std::string array2Name;
      vtkSMProperty* colorArray2Property = this->GetProperty("ColorArray2Name");
      if (colorArray2Property)
      {
        vtkSMPropertyHelper colorArray2Helper(colorArray2Property);
        array2Name = colorArray2Helper.GetInputArrayNameToProcess();
      }
      if (!array2Name.empty())
      {
        ss << "_" << array2Name;
      }
    }
  }
  if (vtkSMPropertyHelper(this, "UseSeparateColorMap", true).GetAsInt())
  {
    // Use global id for separate color map
    std::ostringstream ss1;
    ss1 << "Separate_" << this->GetGlobalIDAsString() << "_" << ss.str();
    return ss1.str();
  }
  return ss.str();
}

//----------------------------------------------------------------------------
int vtkSMPVRepresentationProxy::IsScalarBarStickyVisible(vtkSMProxy* view)
{
  return vtkSMColorMapEditorHelper::IsScalarBarStickyVisible(this, view);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::UpdateScalarBarRange(vtkSMProxy* view, bool deleteRange)
{
  return vtkSMColorMapEditorHelper::UpdateScalarBarRange(this, view, deleteRange);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMPVRepresentationProxy::GetLUTProxy(vtkSMProxy* view)
{
  return vtkSMColorMapEditorHelper::GetLUTProxy(this, view);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::SetScalarBarVisibility(vtkSMProxy* view, bool visible)
{
  return vtkSMColorMapEditorHelper::SetScalarBarVisibility(this, view, visible);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::HideScalarBarIfNotNeeded(vtkSMProxy* view)
{
  return vtkSMColorMapEditorHelper::HideScalarBarIfNotNeeded(this, view);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::IsScalarBarVisible(vtkSMProxy* view)
{
  return vtkSMColorMapEditorHelper::IsScalarBarVisible(this, view);
}

//----------------------------------------------------------------------------
vtkPVArrayInformation* vtkSMPVRepresentationProxy::GetArrayInformationForColorArray(
  bool checkRepresentedData)
{
  return vtkSMColorMapEditorHelper::GetArrayInformationForColorArray(this, checkRepresentedData);
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
    if (vtkSMPropertyHelper(this, "Visibility").GetAsInt() && this->GetUsingScalarColoring())
    {
      this->UpdateScalarBarRange(view, false /* deleteRange */);
    }
    else
    {
      this->UpdateScalarBarRange(view, true /* deleteRange */);
    }
  }
  this->Superclass::ViewUpdated(view);
}
