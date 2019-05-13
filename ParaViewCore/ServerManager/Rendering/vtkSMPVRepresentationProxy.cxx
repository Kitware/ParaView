/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPVRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPVRepresentationProxy.h"

#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVProminentValuesInformation.h"
#include "vtkPVTemporalDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTrace.h"
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

vtkStandardNewMacro(vtkSMPVRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::vtkSMPVRepresentationProxy()
{
  this->SetSIClassName("vtkSIPVRepresentationProxy");
  this->RepresentationSubProxies = new vtkStringSet();
  this->InReadXMLAttributes = false;
}

//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::~vtkSMPVRepresentationProxy()
{
  delete this->RepresentationSubProxies;
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
          vtkSMPropertyHelper(selectionRepr, "Input").Set(esProxy);
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
      child->GetAttribute("subproxy") != NULL)
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
      vtkSMProperty* subProperty = subProxy ? subProxy->GetProperty("Input") : NULL;
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
  if (this->GetProperty("ColorArrayName"))
  {
    vtkSMPropertyHelper helper(this->GetProperty("ColorArrayName"));
    return (helper.GetNumberOfElements() == 5 && helper.GetAsString(4) != NULL &&
      strcmp(helper.GetAsString(4), "") != 0);
  }
  else
  {
    vtkWarningMacro("Missing 'ColorArrayName' property.");
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(bool extend, bool force)
{
  if (!this->GetUsingScalarColoring())
  {
    // we are not using scalar coloring, nothing to do.
    return false;
  }

  SM_SCOPED_TRACE(CallMethod)
    .arg(this)
    .arg("RescaleTransferFunctionToDataRange")
    .arg(extend)
    .arg(force)
    .arg("comment",
      (extend ? "rescale color and/or opacity maps used to include current data range"
              : "rescale color and/or opacity maps used to exactly fit the current data range"));
  return this->RescaleTransferFunctionToDataRange(
    this->GetArrayInformationForColorArray(), extend, force);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(
  const char* arrayname, int attribute_type, bool extend, bool force)
{
  vtkSMPropertyHelper inputHelper(this->GetProperty("Input"));
  vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  int port = inputHelper.GetOutputPort();
  if (!inputProxy)
  {
    // no input.
    vtkWarningMacro("No input present. Cannot determine data ranges.");
    return false;
  }

  vtkPVDataInformation* dataInfo = inputProxy->GetDataInformation(port);
  vtkPVArrayInformation* info = dataInfo->GetArrayInformation(arrayname, attribute_type);
  if (!info)
  {
    vtkPVDataInformation* representedDataInfo = this->GetRepresentedDataInformation();
    info = representedDataInfo->GetArrayInformation(arrayname, attribute_type);
  }

  return this->RescaleTransferFunctionToDataRange(info, extend, force);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRangeOverTime()
{
  if (!this->GetUsingScalarColoring())
  {
    // we are not using scalar coloring, nothing to do.
    return false;
  }

  vtkSMPropertyHelper helper(this->GetProperty("ColorArrayName"));

  return this->RescaleTransferFunctionToDataRangeOverTime(
    helper.GetAsString(4), helper.GetAsInt(3));
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRangeOverTime(
  const char* arrayname, int attribute_type)
{
  vtkSMPropertyHelper inputHelper(this->GetProperty("Input"));
  vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  int port = inputHelper.GetOutputPort();
  if (!inputProxy || !inputProxy->GetOutputPort(port))
  {
    // no input.
    vtkWarningMacro("No input present. Cannot determine data ranges.");
    return false;
  }

  vtkPVTemporalDataInformation* dataInfo =
    inputProxy->GetOutputPort(port)->GetTemporalDataInformation();
  vtkPVArrayInformation* info = dataInfo->GetArrayInformation(arrayname, attribute_type);
  return info ? this->RescaleTransferFunctionToDataRange(info) : false;
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
  if (!lutProperty && !sofProperty)
  {
    vtkWarningMacro("No 'LookupTable' and 'ScalarOpacityFunction' found.");
    return false;
  }

  vtkSMProxy* lut = vtkSMPropertyHelper(lutProperty).GetAsProxy();
  vtkSMProxy* sof = vtkSMPropertyHelper(sofProperty).GetAsProxy();

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

  if (component < info->GetNumberOfComponents())
  {
    int indexedLookup = vtkSMPropertyHelper(lut, "IndexedLookup").GetAsInt();
    if (indexedLookup)
    {
      vtkPVProminentValuesInformation* prominentValues =
        vtkSMPVRepresentationProxy::GetProminentValuesInformationForColorArray(this);
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

      if (this->GetVolumeIndependentRanges())
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
      return (lut || sof);
    }
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToVisibleRange(vtkSMProxy* view)
{
  if (!this->GetUsingScalarColoring())
  {
    // we are not using scalar coloring, nothing to do.
    return false;
  }

  vtkSMPropertyHelper helper(this->GetProperty("ColorArrayName"));
  return this->RescaleTransferFunctionToVisibleRange(
    view, helper.GetAsString(4), helper.GetAsInt(3));
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToVisibleRange(
  vtkSMProxy* view, const char* arrayname, int attribute_type)
{
  vtkSMRenderViewProxy* rview = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!rview || !arrayname || arrayname[0] == 0)
  {
    return false;
  }

  vtkSMPropertyHelper inputHelper(this->GetProperty("Input"));
  vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  int port = inputHelper.GetOutputPort();
  if (!inputProxy || !inputProxy->GetOutputPort(port))
  {
    // no input.
    vtkWarningMacro("No input present. Cannot determine data ranges.");
    return false;
  }

  vtkPVDataInformation* dataInfo = inputProxy->GetOutputPort(port)->GetDataInformation();
  vtkPVArrayInformation* info = dataInfo->GetArrayInformation(arrayname, attribute_type);
  if (!info)
  {
    return false;
  }

  vtkSMProperty* lutProperty = this->GetProperty("LookupTable");
  vtkSMProperty* sofProperty = this->GetProperty("ScalarOpacityFunction");
  if (!lutProperty && !sofProperty)
  {
    // No LookupTable and ScalarOpacityFunction found.
    return false;
  }

  vtkSMProxy* lut = lutProperty ? vtkSMPropertyHelper(lutProperty).GetAsProxy() : NULL;
  vtkSMProxy* sof = sofProperty ? vtkSMPropertyHelper(sofProperty).GetAsProxy() : NULL;

  // We need to determine the component number to use from the lut.
  int component = -1;
  if (lut && vtkSMPropertyHelper(lut, "VectorMode").GetAsInt() != 0)
  {
    component = vtkSMPropertyHelper(lut, "VectorComponent").GetAsInt();
  }
  if (component >= info->GetNumberOfComponents())
  {
    // something amiss, the component request is not present in the dataset.
    // give up.
    return false;
  }

  double range[2];
  if (!rview->ComputeVisibleScalarRange(attribute_type, arrayname, component, range))
  {
    return false;
  }

  if (lut)
  {
    vtkSMTransferFunctionProxy::RescaleTransferFunction(lut, range, false);
    vtkSMProxy* sof_lut = vtkSMPropertyHelper(lut, "ScalarOpacityFunction", true).GetAsProxy();
    if (sof_lut && sof != sof_lut)
    {
      vtkSMTransferFunctionProxy::RescaleTransferFunction(sof_lut, range, false);
    }
  }
  if (sof)
  {
    vtkSMTransferFunctionProxy::RescaleTransferFunction(sof, range, false);
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::SetScalarColoring(const char* arrayname, int attribute_type)
{
  return this->SetScalarColoringInternal(arrayname, attribute_type, false, -1);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::SetScalarColoring(
  const char* arrayname, int attribute_type, int component)
{
  return this->SetScalarColoringInternal(arrayname, attribute_type, true, component);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::SetScalarColoringInternal(
  const char* arrayname, int attribute_type, bool useComponent, int component)
{
  if (!this->GetUsingScalarColoring() && (arrayname == NULL || arrayname[0] == 0))
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

  if (arrayname == NULL || arrayname[0] == '\0')
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

  // Now, setup transfer functions.
  bool haveComponent = useComponent;
  bool separate = (vtkSMPropertyHelper(this, "UseSeparateColorMap", true).GetAsInt() != 0);
  std::string decoratedArrayName = this->GetDecoratedArrayName(arrayname);
  vtkNew<vtkSMTransferFunctionManager> mgr;
  if (vtkSMProperty* lutProperty = this->GetProperty("LookupTable"))
  {
    vtkSMProxy* lutProxy =
      mgr->GetColorTransferFunction(decoratedArrayName.c_str(), this->GetSessionProxyManager());
    if (useComponent)
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

  this->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
std::string vtkSMPVRepresentationProxy::GetDecoratedArrayName(const std::string& arrayname)
{
  if (vtkSMPropertyHelper(this, "UseSeparateColorMap", true).GetAsInt())
  {
    // Use global id for separate color map
    std::ostringstream ss;
    ss << "Separate_" << this->GetGlobalIDAsString() << "_" << arrayname;
    return ss.str();
  }
  return arrayname;
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::SetScalarBarVisibility(vtkSMProxy* view, bool visible)
{
  if (!view)
  {
    return false;
  }

  vtkSMProperty* lutProperty = this->GetProperty("LookupTable");
  if (!lutProperty)
  {
    vtkWarningMacro("Missing 'LookupTable' property.");
    return false;
  }

  vtkSMPropertyHelper lutPropertyHelper(lutProperty);
  if (lutPropertyHelper.GetNumberOfElements() == 0 || lutPropertyHelper.GetAsProxy(0) == NULL)
  {
    vtkWarningMacro("Failed to determine the LookupTable being used.");
    return false;
  }

  SM_SCOPED_TRACE(CallMethod)
    .arg(this)
    .arg("SetScalarBarVisibility")
    .arg(view)
    .arg(visible)
    .arg("comment", visible ? "show color bar/color legend" : "hide color bar/color legend");

  vtkSMProxy* lutProxy = lutPropertyHelper.GetAsProxy(0);

  // if hiding the Scalar Bar, just look if there's a LUT and then hide the
  // corresponding scalar bar. We won't worry too much about whether scalar
  // coloring is currently enabled for this.
  if (!visible)
  {
    if (vtkSMProxy* sbProxy = vtkSMTransferFunctionProxy::FindScalarBarRepresentation(
          lutPropertyHelper.GetAsProxy(), view))
    {
      vtkSMPropertyHelper(sbProxy, "Visibility").Set(0);
      vtkSMPropertyHelper(sbProxy, "Enabled").Set(0);
      sbProxy->UpdateVTKObjects();
    }
    return true;
  }

  if (!this->GetUsingScalarColoring())
  {
    return false;
  }

  vtkNew<vtkSMTransferFunctionManager> mgr;
  vtkSMProxy* sbProxy = mgr->GetScalarBarRepresentation(lutProxy, view);
  if (!sbProxy)
  {
    vtkWarningMacro("Failed to locate/create ScalarBar representation.");
    return false;
  }

  vtkSMPropertyHelper(sbProxy, "Enabled").Set(1);
  vtkSMPropertyHelper(sbProxy, "Visibility").Set(1);

  vtkSMProperty* titleProp = sbProxy->GetProperty("Title");
  vtkSMProperty* compProp = sbProxy->GetProperty("ComponentTitle");
  if (titleProp && compProp && titleProp->IsValueDefault() && compProp->IsValueDefault())
  {
    vtkSMPropertyHelper colorArrayHelper(this, "ColorArrayName");
    vtkSMPropertyHelper(titleProp).Set(colorArrayHelper.GetInputArrayNameToProcess());
    // now, determine a name for it if possible.
    vtkPVArrayInformation* arrayInfo = this->GetArrayInformationForColorArray();
    vtkSMScalarBarWidgetRepresentationProxy::UpdateComponentTitle(sbProxy, arrayInfo);
  }
  vtkSMScalarBarWidgetRepresentationProxy::PlaceInView(sbProxy, view);
  sbProxy->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::HideScalarBarIfNotNeeded(vtkSMProxy* view)
{
  vtkSMProperty* lutProperty = this->GetProperty("LookupTable");
  if (!lutProperty)
  {
    return false;
  }

  vtkSMPropertyHelper lutPropertyHelper(lutProperty);
  if (lutPropertyHelper.GetNumberOfElements() == 0 || lutPropertyHelper.GetAsProxy(0) == NULL)
  {
    return false;
  }

  SM_SCOPED_TRACE(CallMethod)
    .arg(this)
    .arg("HideScalarBarIfNotNeeded")
    .arg(view)
    .arg("comment", "hide scalars not actively used");

  vtkSMProxy* lutProxy = lutPropertyHelper.GetAsProxy(0);
  vtkNew<vtkSMTransferFunctionManager> tmgr;
  return tmgr->HideScalarBarIfNotNeeded(lutProxy, view);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::IsScalarBarVisible(vtkSMProxy* view)
{
  vtkSMProperty* lutProperty = this->GetProperty("LookupTable");
  if (!lutProperty)
  {
    return false;
  }

  vtkSMPropertyHelper lutPropertyHelper(lutProperty);
  if (lutPropertyHelper.GetNumberOfElements() == 0 || lutPropertyHelper.GetAsProxy(0) == NULL)
  {
    return false;
  }

  vtkSMProxy* lutProxy = lutPropertyHelper.GetAsProxy(0);
  return vtkSMTransferFunctionProxy::IsScalarBarVisible(lutProxy, view);
}

//----------------------------------------------------------------------------
vtkPVArrayInformation* vtkSMPVRepresentationProxy::GetArrayInformationForColorArray()
{
  if (!this->GetUsingScalarColoring())
  {
    return NULL;
  }

  // now, determine a name for it if possible.
  vtkSMPropertyHelper colorArrayHelper(this, "ColorArrayName");
  vtkSMPropertyHelper inputHelper(this, "Input");
  vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  unsigned int port = inputHelper.GetOutputPort();
  if (input)
  {
    vtkPVArrayInformation* arrayInfoFromData = input->GetDataInformation(port)->GetArrayInformation(
      colorArrayHelper.GetInputArrayNameToProcess(), colorArrayHelper.GetInputArrayAssociation());
    if (arrayInfoFromData)
    {
      return arrayInfoFromData;
    }
  }

  vtkPVArrayInformation* arrayInfo = this->GetRepresentedDataInformation()->GetArrayInformation(
    colorArrayHelper.GetInputArrayNameToProcess(), colorArrayHelper.GetInputArrayAssociation());
  if (arrayInfo)
  {
    return arrayInfo;
  }

  return NULL;
}

//----------------------------------------------------------------------------
vtkPVProminentValuesInformation*
vtkSMPVRepresentationProxy::GetProminentValuesInformationForColorArray(
  double uncertaintyAllowed, double fraction, bool force)
{
  if (!this->GetUsingScalarColoring())
  {
    return NULL;
  }

  vtkPVArrayInformation* arrayInfo = this->GetArrayInformationForColorArray();
  if (!arrayInfo)
  {
    return NULL;
  }

  vtkSMPropertyHelper colorArrayHelper(this, "ColorArrayName");
  return this->GetProminentValuesInformation(arrayInfo->GetName(),
    colorArrayHelper.GetInputArrayAssociation(), arrayInfo->GetNumberOfComponents(),
    uncertaintyAllowed, fraction, force);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::SetRepresentationType(const char* type)
{
  const int CALL_SUPERCLASS = 0;
  try
  {
    if (type == 0)
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
  catch (int val)
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
  if (!view)
  {
    return -1;
  }

  if (!this->GetUsingScalarColoring())
  {
    return 0;
  }

  vtkSMProperty* lutProperty = this->GetProperty("LookupTable");
  if (!lutProperty)
  {
    vtkWarningMacro("Missing 'LookupTable' property.");
    return -1;
  }

  vtkSMPropertyHelper lutPropertyHelper(lutProperty);
  if (lutPropertyHelper.GetNumberOfElements() == 0 || lutPropertyHelper.GetAsProxy(0) == NULL)
  {
    vtkWarningMacro("Failed to determine the LookupTable being used.");
    return -1;
  }

  vtkSMProxy* lutProxy = lutPropertyHelper.GetAsProxy(0);
  vtkNew<vtkSMTransferFunctionManager> mgr;
  vtkSMProxy* sbProxy = mgr->GetScalarBarRepresentation(lutProxy, view);
  if (!sbProxy)
  {
    vtkWarningMacro("Failed to locate/create ScalarBar representation.");
    return -1;
  }

  sbProxy->UpdatePropertyInformation();
  return vtkSMPropertyHelper(sbProxy, "EstimatedNumberOfAnnotations").GetAsInt();
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::GetVolumeIndependentRanges()
{
  // the representation is Volume
  vtkSMProperty* repProperty = this->GetProperty("Representation");
  if (strcmp(vtkSMPropertyHelper(repProperty).GetAsString(), "Volume") != 0)
  {
    return false;
  }

  // MapScalars and MultiComponentsMapping are checked
  vtkSMProperty* msProperty = this->GetProperty("MapScalars");
  vtkSMProperty* mcmProperty = this->GetProperty("MultiComponentsMapping");
  return (vtkSMPropertyHelper(msProperty).GetAsInt() != 0 &&
    vtkSMPropertyHelper(mcmProperty).GetAsInt() != 0);
}
