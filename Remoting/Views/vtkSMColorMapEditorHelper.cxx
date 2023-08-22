// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMColorMapEditorHelper.h"

#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVProminentValuesInformation.h"
#include "vtkPVRepresentedArrayListSettings.h"
#include "vtkPVTemporalDataInformation.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"
#include "vtkSMSettings.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTransferFunction2DProxy.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkStringList.h"

#include <string>

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::GetUsingScalarColoring(vtkSMProxy* proxy)
{
  if (!vtkSMRepresentationProxy::SafeDownCast(proxy))
  {
    return false;
  }

  if (proxy->GetProperty("ColorArrayName"))
  {
    vtkSMPropertyHelper helper(proxy->GetProperty("ColorArrayName"));
    return (helper.GetNumberOfElements() == 5 && helper.GetAsString(4) &&
      strcmp(helper.GetAsString(4), "") != 0);
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetupLookupTable(vtkSMProxy* proxy)
{
  if (vtkSMColorMapEditorHelper::GetUsingScalarColoring(proxy))
  {
    // If representation has been initialized to use scalar coloring and no
    // transfer functions are setup, we setup the transfer functions.
    vtkSMPropertyHelper helper(proxy, "ColorArrayName");
    const char* arrayName = helper.GetInputArrayNameToProcess();
    if (arrayName && arrayName[0] != '\0')
    {
      vtkNew<vtkSMTransferFunctionManager> mgr;
      if (vtkSMProperty* sofProperty = proxy->GetProperty("ScalarOpacityFunction"))
      {
        vtkSMProxy* sofProxy =
          mgr->GetOpacityTransferFunction(arrayName, proxy->GetSessionProxyManager());
        vtkSMPropertyHelper(sofProperty).Set(sofProxy);
      }
      if (vtkSMProperty* lutProperty = proxy->GetProperty("LookupTable"))
      {
        vtkSMTransferFunctionProxy* lutProxy = vtkSMTransferFunctionProxy::SafeDownCast(
          mgr->GetColorTransferFunction(arrayName, proxy->GetSessionProxyManager()));
        int rescaleMode =
          vtkSMPropertyHelper(lutProxy, "AutomaticRescaleRangeMode", true).GetAsInt();
        vtkSMPropertyHelper(lutProperty).Set(lutProxy);
        bool extend = rescaleMode == vtkSMTransferFunctionManager::GROW_ON_APPLY;
        bool force = false;
        vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(proxy, extend, force);
        proxy->UpdateVTKObjects();
      }
    }
  }
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(
  vtkSMProxy* proxy, bool extend, bool force)
{
  SM_SCOPED_TRACE(CallMethod)
    .arg(proxy)
    .arg("RescaleTransferFunctionToDataRange")
    .arg(extend)
    .arg(force)
    .arg("comment",
      (extend ? "rescale color and/or opacity maps used to include current data range"
              : "rescale color and/or opacity maps used to exactly fit the current data range"));
  return RescaleTransferFunctionToDataRange(
    proxy, GetArrayInformationForColorArray(proxy), extend, force);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(
  vtkSMProxy* proxy, const char* arrayname, int attribute_type, bool extend, bool force)
{
  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  if (!repr)
  {
    return false;
  }

  vtkSMPropertyHelper inputHelper(proxy->GetProperty("Input"));
  vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  int port = inputHelper.GetOutputPort();
  if (!inputProxy)
  {
    // no input.
    vtkGenericWarningMacro("No input present. Cannot determine data ranges.");
    return false;
  }

  vtkPVDataInformation* dataInfo = inputProxy->GetDataInformation(port);
  vtkPVArrayInformation* info = dataInfo->GetArrayInformation(arrayname, attribute_type);
  if (!info)
  {
    vtkPVDataInformation* representedDataInfo = repr->GetRepresentedDataInformation();
    info = representedDataInfo->GetArrayInformation(arrayname, attribute_type);
  }

  return RescaleTransferFunctionToDataRange(proxy, info, extend, force);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRangeOverTime(vtkSMProxy* proxy)
{
  vtkSMPropertyHelper helper(proxy->GetProperty("ColorArrayName"));

  return RescaleTransferFunctionToDataRangeOverTime(
    proxy, helper.GetAsString(4), helper.GetAsInt(3));
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRangeOverTime(
  vtkSMProxy* proxy, const char* arrayname, int attribute_type)
{
  vtkSMPropertyHelper inputHelper(proxy->GetProperty("Input"));
  vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  int port = inputHelper.GetOutputPort();
  if (!inputProxy || !inputProxy->GetOutputPort(port))
  {
    // no input.
    vtkGenericWarningMacro("No input present. Cannot determine data ranges.");
    return false;
  }

  vtkPVTemporalDataInformation* dataInfo =
    inputProxy->GetOutputPort(port)->GetTemporalDataInformation();
  vtkPVArrayInformation* info = dataInfo->GetArrayInformation(arrayname, attribute_type);
  return info ? RescaleTransferFunctionToDataRange(proxy, info) : false;
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(
  vtkSMProxy* proxy, vtkPVArrayInformation* info, bool extend, bool force)
{
  if (!GetUsingScalarColoring(proxy))
  {
    // we are not using scalar coloring, nothing to do.
    return false;
  }

  if (!info)
  {
    vtkGenericWarningMacro("Could not determine array range.");
    return false;
  }

  vtkSMProperty* lutProperty = proxy->GetProperty("LookupTable");
  vtkSMProperty* sofProperty = proxy->GetProperty("ScalarOpacityFunction");
  vtkSMProperty* tf2dProperty = proxy->GetProperty("TransferFunction2D");
  if (!lutProperty && !sofProperty && !tf2dProperty)
  {
    vtkGenericWarningMacro(
      "No 'LookupTable', 'ScalarOpacityFunction' and 'TransferFunction2D' found.");
    return false;
  }

  vtkSMProxy* lut = vtkSMPropertyHelper(lutProperty, true).GetAsProxy();
  vtkSMProxy* sof = vtkSMPropertyHelper(sofProperty, true).GetAsProxy();
  vtkSMProxy* tf2d = vtkSMPropertyHelper(tf2dProperty, true).GetAsProxy();

  if (!force && lut &&
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
    if (indexedLookup > 0)
    {
      vtkPVProminentValuesInformation* prominentValues =
        vtkSMColorMapEditorHelper::GetProminentValuesInformationForColorArray(proxy);
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
      if (auto uoaProperty = proxy->GetProperty("UseSeparateOpacityArray"))
      {
        useOpacityArray = vtkSMPropertyHelper(uoaProperty).GetAsInt() == 1;
      }

      vtkSMPVRepresentationProxy* pvRepr = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
      if (useOpacityArray)
      {
        vtkSMPropertyHelper inputHelper(proxy->GetProperty("Input"));
        vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
        int port = inputHelper.GetOutputPort();
        if (!inputProxy)
        {
          // no input.
          vtkGenericWarningMacro("No input present. Cannot determine opacity data range.");
          return false;
        }

        vtkPVDataInformation* dataInfo = inputProxy->GetDataInformation(port);
        vtkSMPropertyHelper opacityArrayNameHelper(proxy, "OpacityArrayName");
        int opacityArrayFieldAssociation = opacityArrayNameHelper.GetAsInt(3);
        const char* opacityArrayName = opacityArrayNameHelper.GetAsString(4);
        int opacityArrayComponent = vtkSMPropertyHelper(proxy, "OpacityComponent").GetAsInt();

        vtkPVArrayInformation* opacityArrayInfo =
          dataInfo->GetArrayInformation(opacityArrayName, opacityArrayFieldAssociation);
        if (opacityArrayComponent >= opacityArrayInfo->GetNumberOfComponents())
        {
          opacityArrayComponent = -1;
        }
        info->GetComponentFiniteRange(component, rangeColor);
        opacityArrayInfo->GetComponentFiniteRange(opacityArrayComponent, rangeOpacity);
      }
      else if (pvRepr && pvRepr->GetVolumeIndependentRanges())
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
bool vtkSMColorMapEditorHelper::RescaleTransferFunctionToVisibleRange(
  vtkSMProxy* proxy, vtkSMProxy* view)
{
  vtkSMPropertyHelper helper(proxy->GetProperty("ColorArrayName"));
  return RescaleTransferFunctionToVisibleRange(
    proxy, view, helper.GetAsString(4), helper.GetAsInt(3));
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::RescaleTransferFunctionToVisibleRange(
  vtkSMProxy* proxy, vtkSMProxy* view, const char* arrayname, int attribute_type)
{
  if (!GetUsingScalarColoring(proxy))
  {
    // we are not using scalar coloring, nothing to do.
    return false;
  }

  // TODO: Add option for charts
  vtkSMRenderViewProxy* rview = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!rview || !arrayname || arrayname[0] == '\0')
  {
    return false;
  }

  vtkSMPropertyHelper inputHelper(proxy->GetProperty("Input"));
  vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  int port = inputHelper.GetOutputPort();
  if (!inputProxy || !inputProxy->GetOutputPort(port))
  {
    // no input.
    vtkGenericWarningMacro("No input present. Cannot determine data ranges.");
    return false;
  }

  vtkPVDataInformation* dataInfo = inputProxy->GetOutputPort(port)->GetDataInformation();
  vtkPVArrayInformation* info = dataInfo->GetArrayInformation(arrayname, attribute_type);
  if (!info)
  {
    return false;
  }

  vtkSMProperty* lutProperty = proxy->GetProperty("LookupTable");
  vtkSMProperty* sofProperty = proxy->GetProperty("ScalarOpacityFunction");
  vtkSMProperty* useTransfer2DProperty = proxy->GetProperty("UseTransfer2D");
  vtkSMProperty* transfer2DProperty = proxy->GetProperty("TransferFunction2D");
  if ((!lutProperty && !sofProperty) && (!useTransfer2DProperty && !transfer2DProperty))
  {
    // No LookupTable and ScalarOpacityFunction found.
    // No UseTransfer2D and TransferFunction2D found.
    return false;
  }

  vtkSMProxy* lut = lutProperty ? vtkSMPropertyHelper(lutProperty).GetAsProxy() : nullptr;
  vtkSMProxy* sof = sofProperty ? vtkSMPropertyHelper(sofProperty).GetAsProxy() : nullptr;
  bool useTransfer2D =
    useTransfer2DProperty ? vtkSMPropertyHelper(useTransfer2DProperty).GetAsInt() == 1 : false;
  vtkSMProxy* tf2d = (useTransfer2D && transfer2DProperty)
    ? vtkSMPropertyHelper(transfer2DProperty).GetAsProxy()
    : nullptr;

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

  if (!useTransfer2D)
  {
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
  }
  else
  {
    if (tf2d)
    {
      double r[4];
      vtkSMTransferFunction2DProxy::GetRange(tf2d, r);
      r[0] = range[0];
      r[1] = range[1];
      vtkSMTransferFunction2DProxy::RescaleTransferFunction(tf2d, r, false);
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::SetScalarColoring(
  vtkSMProxy* proxy, const char* arrayname, int attribute_type)
{
  return SetScalarColoringInternal(proxy, arrayname, attribute_type, false, -1);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::SetScalarColoring(
  vtkSMProxy* proxy, const char* arrayname, int attribute_type, int component)
{
  return SetScalarColoringInternal(proxy, arrayname, attribute_type, true, component);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::SetScalarColoringInternal(
  vtkSMProxy* proxy, const char* arrayname, int attribute_type, bool useComponent, int component)
{
  if (!GetUsingScalarColoring(proxy) && (!arrayname || arrayname[0] == '\0'))
  {
    // if true, scalar coloring already off: Nothing to do.
    return true;
  }

  vtkSMProperty* colorArray = proxy->GetProperty("ColorArrayName");
  vtkSMPropertyHelper colorArrayHelper(colorArray);
  colorArrayHelper.SetInputArrayToProcess(attribute_type, arrayname);

  if (!arrayname || arrayname[0] == '\0')
  {
    SM_SCOPED_TRACE(SetScalarColoring)
      .arg("display", proxy)
      .arg("arrayname", arrayname)
      .arg("attribute_type", attribute_type);
    vtkSMPropertyHelper(proxy, "LookupTable", true).RemoveAllValues();
    vtkSMPropertyHelper(proxy, "ScalarOpacityFunction", true).RemoveAllValues();
    proxy->UpdateVTKObjects();
    return true;
  }

  auto* arraySettings = vtkPVRepresentedArrayListSettings::GetInstance();
  auto* arrayInfo = GetArrayInformationForColorArray(proxy, false);
  const bool forceComponentMode = (arrayInfo && arraySettings &&
    !arraySettings->ShouldUseMagnitudeMode(arrayInfo->GetNumberOfComponents()));
  if (forceComponentMode && (!useComponent || component < 0))
  {
    component = 0;
  }

  // Now, setup transfer functions.
  bool haveComponent = useComponent;
  bool separate = (vtkSMPropertyHelper(proxy, "UseSeparateColorMap", true).GetAsInt() != 0);
  bool useTransfer2D = (vtkSMPropertyHelper(proxy, "UseTransfer2D", true).GetAsInt() != 0);
  std::string decoratedArrayName = GetDecoratedArrayName(proxy, arrayname);
  vtkNew<vtkSMTransferFunctionManager> mgr;
  vtkSMProxy* lutProxy = nullptr;
  if (vtkSMProperty* lutProperty = proxy->GetProperty("LookupTable"))
  {
    lutProxy =
      mgr->GetColorTransferFunction(decoratedArrayName.c_str(), proxy->GetSessionProxyManager());
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
    vtkPVArrayInformation* colorArrayInfo = GetArrayInformationForColorArray(proxy);
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
            .arg("display", proxy)
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
      .arg("display", proxy)
      .arg("arrayname", arrayname)
      .arg("attribute_type", attribute_type)
      .arg("separate", separate);
  }

  if (vtkSMProperty* sofProperty = proxy->GetProperty("ScalarOpacityFunction"))
  {
    vtkSMProxy* sofProxy =
      mgr->GetOpacityTransferFunction(decoratedArrayName.c_str(), proxy->GetSessionProxyManager());
    vtkSMPropertyHelper(sofProperty).Set(sofProxy);
  }

  if (vtkSMProperty* tf2dProperty = proxy->GetProperty("TransferFunction2D"))
  {
    vtkSMProxy* tf2dProxy =
      mgr->GetTransferFunction2D(decoratedArrayName.c_str(), proxy->GetSessionProxyManager());
    vtkSMPropertyHelper(tf2dProperty).Set(tf2dProxy);
    proxy->UpdateProperty("TransferFunction2D");
    if (lutProxy && useTransfer2D)
    {
      vtkSMPropertyHelper(lutProxy, "Using2DTransferFunction").Set(useTransfer2D);
      lutProxy->UpdateVTKObjects();
    }
  }

  proxy->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
std::string vtkSMColorMapEditorHelper::GetDecoratedArrayName(
  vtkSMProxy* proxy, const std::string& arrayname)
{
  std::ostringstream ss;
  ss << arrayname;
  if (proxy->GetProperty("TransferFunction2D"))
  {
    bool useGradientAsY =
      (vtkSMPropertyHelper(proxy, "UseGradientForTransfer2D", true).GetAsInt() == 1);
    if (!useGradientAsY)
    {
      std::string array2Name;
      vtkSMProperty* colorArray2Property = proxy->GetProperty("ColorArray2Name");
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
  if (vtkSMPropertyHelper(proxy, "UseSeparateColorMap", true).GetAsInt())
  {
    // Use global id for separate color map
    std::ostringstream ss1;
    ss1 << "Separate_" << proxy->GetGlobalIDAsString() << "_" << ss.str();
    return ss1.str();
  }
  return ss.str();
}

//----------------------------------------------------------------------------
int vtkSMColorMapEditorHelper::IsScalarBarStickyVisible(vtkSMProxy* proxy, vtkSMProxy* view)
{
  if (!view)
  {
    return -1;
  }
  vtkSMProperty* lutProperty = proxy->GetProperty("LookupTable");
  if (!lutProperty)
  {
    vtkGenericWarningMacro("Missing 'LookupTable' property.");
    return -1;
  }
  vtkSMPropertyHelper lutPropertyHelper(lutProperty);
  if (lutPropertyHelper.GetNumberOfElements() == 0 || !lutPropertyHelper.GetAsProxy())
  {
    vtkGenericWarningMacro("Failed to determine the LookupTable being used.");
    return -1;
  }
  vtkSMProxy* lutProxy = lutPropertyHelper.GetAsProxy();
  vtkSMScalarBarWidgetRepresentationProxy* sbProxy =
    vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
      vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lutProxy, view));
  if (sbProxy)
  {
    vtkSMPropertyHelper sbsvPropertyHelper(sbProxy->GetProperty("StickyVisible"));
    return sbsvPropertyHelper.GetNumberOfElements()
      ? vtkSMPropertyHelper(sbProxy, "StickyVisible").GetAsInt()
      : -1;
  }
  return -1;
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::UpdateScalarBarRange(
  vtkSMProxy* proxy, vtkSMProxy* view, bool deleteRange)
{
  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  if (!repr)
  {
    return false;
  }

  bool usingScalarBarColoring = GetUsingScalarColoring(proxy);

  vtkSMProperty* lutProperty = proxy->GetProperty("LookupTable");
  if (!lutProperty)
  {
    vtkGenericWarningMacro("Missing 'LookupTable' property");
    return false;
  }

  vtkSMPropertyHelper lutPropertyHelper(lutProperty);
  vtkSMProxy* lutProxy =
    usingScalarBarColoring ? lutPropertyHelper.GetAsProxy() : GetLastLUTProxy(proxy);

  if (!lutProxy)
  {
    return false;
  }

  vtkSMScalarBarWidgetRepresentationProxy* sbProxy =
    vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
      vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lutProxy, view));

  if (!sbProxy)
  {
    return false;
  }

  vtkNew<vtkSMTransferFunctionManager> mgr;
  vtkSMProxy* sbSMProxy = mgr->GetScalarBarRepresentation(lutProxy, view);

  vtkSMProperty* maxRangeProp = sbSMProxy->GetProperty("DataRangeMax");
  vtkSMProperty* minRangeProp = sbSMProxy->GetProperty("DataRangeMin");

  if (minRangeProp && maxRangeProp)
  {
    vtkSMPropertyHelper minRangePropHelper(minRangeProp);
    vtkSMPropertyHelper maxRangePropHelper(maxRangeProp);

    // We remove the range that was potentially previously stored
    sbProxy->RemoveRange(repr);

    // If we do not want to delete proxy range, then we update it with its potential new value.
    if (!deleteRange)
    {
      sbProxy->AddRange(repr);
    }

    double updatedRange[2];
    sbProxy->GetRange(updatedRange);
    minRangePropHelper.Set(updatedRange[0]);
    maxRangePropHelper.Set(updatedRange[1]);
  }
  sbSMProxy->UpdateVTKObjects();

  SetLastLUTProxy(proxy, usingScalarBarColoring ? lutProxy : nullptr);
  return true;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMColorMapEditorHelper::GetLUTProxy(vtkSMProxy* proxy, vtkSMProxy* view)
{
  if (!view)
  {
    return nullptr;
  }

  vtkSMProperty* lutProperty = proxy->GetProperty("LookupTable");
  if (!lutProperty)
  {
    vtkGenericWarningMacro("Missing 'LookupTable' property.");
    return nullptr;
  }

  vtkSMPropertyHelper lutPropertyHelper(lutProperty);
  if (lutPropertyHelper.GetNumberOfElements() == 0)
  {
    return nullptr;
  }

  return lutPropertyHelper.GetAsProxy();
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::SetScalarBarVisibility(
  vtkSMProxy* proxy, vtkSMProxy* view, bool visible)
{
  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  if (!repr || !view)
  {
    return false;
  }

  vtkSMProxy* lutProxy = GetLUTProxy(proxy, view);
  if (!lutProxy)
  {
    vtkGenericWarningMacro("Failed to determine the LookupTable being used.");
    return false;
  }

  SM_SCOPED_TRACE(CallMethod)
    .arg(proxy)
    .arg("SetScalarBarVisibility")
    .arg(view)
    .arg(visible)
    .arg("comment", visible ? "show color bar/color legend" : "hide color bar/color legend");

  // If the lut proxy changed, we need to remove ourself (representation proxy)
  // from the scalar bar widget that we used to be linked to.
  vtkSMProxy* lastLUTProxy = GetLastLUTProxy(proxy);
  if (lastLUTProxy && lutProxy != lastLUTProxy)
  {
    if (auto lastSBProxy = vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
          vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lastLUTProxy, view)))
    {
      lastSBProxy->RemoveRange(repr);
    }
  }

  vtkSMScalarBarWidgetRepresentationProxy* sbProxy =
    vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
      vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lutProxy, view));

  if (sbProxy)
  {
    int legacyVisible = vtkSMPropertyHelper(sbProxy, "Visibility").GetAsInt();
    // If scalar bar is set to not be visible but was previously visible,
    // then the user has pressed the scalar bar button hiding the scalar bar.
    // We keep this information well preserved in the scalar bar representation.
    // This method is not called when hiding the whole representation, so we are safe
    // on scalar bar disappearance occuring on such event, it won't mess with this engine.
    if (legacyVisible && !visible)
    {
      vtkSMPropertyHelper(sbProxy, "StickyVisible").Set(0);
    }
    // If the scalar bar is set to be visible, we are in the case where we automatically
    // show the scalarbar, whether or not it was previously hidden.
    else if (visible)
    {
      vtkSMPropertyHelper(sbProxy, "StickyVisible").Set(1);
    }
  }

  // if hiding the Scalar Bar, just look if there's a LUT and then hide the
  // corresponding scalar bar. We won't worry too much about whether scalar
  // coloring is currently enabled for this.
  if (!visible)
  {
    if (sbProxy)
    {
      vtkSMPropertyHelper(sbProxy, "Visibility").Set(0);
      vtkSMPropertyHelper(sbProxy, "Enabled").Set(0);
      sbProxy->UpdateVTKObjects();
    }
    return true;
  }

  if (!GetUsingScalarColoring(proxy))
  {
    return false;
  }

  vtkNew<vtkSMTransferFunctionManager> mgr;
  vtkSMProxy* sbSMProxy = mgr->GetScalarBarRepresentation(lutProxy, view);
  if (!sbSMProxy)
  {
    vtkGenericWarningMacro("Failed to locate/create ScalarBar representation.");
    return false;
  }

  vtkSMPropertyHelper(sbSMProxy, "Enabled").Set(1);
  vtkSMPropertyHelper(sbSMProxy, "Visibility").Set(1);

  vtkSMProperty* titleProp = sbSMProxy->GetProperty("Title");
  vtkSMProperty* compProp = sbSMProxy->GetProperty("ComponentTitle");

  if (titleProp && compProp && titleProp->IsValueDefault() && compProp->IsValueDefault())
  {
    vtkSMSettings* settings = vtkSMSettings::GetInstance();

    vtkSMPropertyHelper colorArrayHelper(proxy, "ColorArrayName");
    std::string arrayName(colorArrayHelper.GetInputArrayNameToProcess());

    // Look up array-specific title for scalar bar
    std::ostringstream prefix;
    prefix << ".array_lookup_tables"
           << "." << arrayName << ".Title";

    std::string arrayTitle = settings->GetSettingAsString(prefix.str().c_str(), arrayName);

    vtkSMPropertyHelper titlePropHelper(titleProp);

    if (sbProxy && strcmp(titlePropHelper.GetAsString(), arrayTitle.c_str()) != 0)
    {
      sbProxy->ClearRange();
    }

    titlePropHelper.Set(arrayTitle.c_str());

    // now, determine a name for it if possible.
    vtkPVArrayInformation* arrayInfo = GetArrayInformationForColorArray(proxy);
    vtkSMScalarBarWidgetRepresentationProxy::UpdateComponentTitle(sbSMProxy, arrayInfo);
  }
  vtkSMScalarBarWidgetRepresentationProxy::PlaceInView(sbSMProxy, view);
  sbSMProxy->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::HideScalarBarIfNotNeeded(vtkSMProxy* proxy, vtkSMProxy* view)
{
  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  vtkSMProperty* lutProperty = proxy->GetProperty("LookupTable");
  if (!repr || !lutProperty)
  {
    return false;
  }

  vtkSMPropertyHelper lutPropertyHelper(lutProperty);
  if (lutPropertyHelper.GetNumberOfElements() == 0 || !lutPropertyHelper.GetAsProxy())
  {
    return false;
  }

  SM_SCOPED_TRACE(CallMethod)
    .arg(proxy)
    .arg("HideScalarBarIfNotNeeded")
    .arg(view)
    .arg("comment", "hide scalars not actively used");

  vtkSMProxy* lutProxy = lutPropertyHelper.GetAsProxy();
  vtkNew<vtkSMTransferFunctionManager> tmgr;
  return tmgr->HideScalarBarIfNotNeeded(lutProxy, view);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::IsScalarBarVisible(vtkSMProxy* proxy, vtkSMProxy* view)
{
  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  vtkSMProperty* lutProperty = proxy->GetProperty("LookupTable");
  if (!repr || !lutProperty)
  {
    return false;
  }

  vtkSMPropertyHelper lutPropertyHelper(lutProperty);
  if (lutPropertyHelper.GetNumberOfElements() == 0 || !lutPropertyHelper.GetAsProxy())
  {
    return false;
  }

  vtkSMProxy* lutProxy = lutPropertyHelper.GetAsProxy();
  return vtkSMTransferFunctionProxy::IsScalarBarVisible(lutProxy, view);
}

//----------------------------------------------------------------------------
vtkPVArrayInformation* vtkSMColorMapEditorHelper::GetArrayInformationForColorArray(
  vtkSMProxy* proxy, bool checkRepresentedData)
{
  if (!GetUsingScalarColoring(proxy))
  {
    return nullptr;
  }

  // now, determine a name for it if possible.
  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  vtkSMPropertyHelper colorArrayHelper(proxy, "ColorArrayName");
  vtkSMPropertyHelper inputHelper(proxy, "Input");
  vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  unsigned int port = inputHelper.GetOutputPort();
  if (input)
  {
    vtkPVArrayInformation* arrayInfoFromData = nullptr;
    arrayInfoFromData = input->GetDataInformation(port)->GetArrayInformation(
      colorArrayHelper.GetInputArrayNameToProcess(), colorArrayHelper.GetInputArrayAssociation());
    if (arrayInfoFromData)
    {
      return arrayInfoFromData;
    }

    if (colorArrayHelper.GetInputArrayAssociation() == vtkDataObject::POINT_THEN_CELL)
    {
      // Try points...
      arrayInfoFromData = input->GetDataInformation(port)->GetArrayInformation(
        colorArrayHelper.GetInputArrayNameToProcess(), vtkDataObject::POINT);
      if (arrayInfoFromData)
      {
        return arrayInfoFromData;
      }

      // ... then cells
      arrayInfoFromData = input->GetDataInformation(port)->GetArrayInformation(
        colorArrayHelper.GetInputArrayNameToProcess(), vtkDataObject::CELL);
      if (arrayInfoFromData)
      {
        return arrayInfoFromData;
      }
    }
  }

  if (checkRepresentedData)
  {
    vtkPVArrayInformation* arrayInfo = repr->GetRepresentedDataInformation()->GetArrayInformation(
      colorArrayHelper.GetInputArrayNameToProcess(), colorArrayHelper.GetInputArrayAssociation());
    if (arrayInfo)
    {
      return arrayInfo;
    }
  }

  return nullptr;
}

//----------------------------------------------------------------------------
vtkPVProminentValuesInformation*
vtkSMColorMapEditorHelper::GetProminentValuesInformationForColorArray(
  vtkSMProxy* proxy, double uncertaintyAllowed, double fraction, bool force)
{
  if (!GetUsingScalarColoring(proxy))
  {
    return nullptr;
  }

  vtkPVArrayInformation* arrayInfo = GetArrayInformationForColorArray(proxy);
  if (!arrayInfo)
  {
    return nullptr;
  }

  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  vtkSMPropertyHelper colorArrayHelper(proxy, "ColorArrayName");

  return repr->GetProminentValuesInformation(arrayInfo->GetName(),
    colorArrayHelper.GetInputArrayAssociation(), arrayInfo->GetNumberOfComponents(),
    uncertaintyAllowed, fraction, force);
}

//----------------------------------------------------------------------------
int vtkSMColorMapEditorHelper::GetEstimatedNumberOfAnnotationsOnScalarBar(
  vtkSMProxy* proxy, vtkSMProxy* view)
{
  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  if (!repr || !view)
  {
    return -1;
  }

  if (!GetUsingScalarColoring(proxy))
  {
    return 0;
  }

  vtkSMProperty* lutProperty = proxy->GetProperty("LookupTable");
  if (!lutProperty)
  {
    vtkGenericWarningMacro("Missing 'LookupTable' property.");
    return -1;
  }

  vtkSMPropertyHelper lutPropertyHelper(lutProperty);
  if (lutPropertyHelper.GetNumberOfElements() == 0 || !lutPropertyHelper.GetAsProxy())
  {
    vtkGenericWarningMacro("Failed to determine the LookupTable being used.");
    return -1;
  }

  vtkSMProxy* lutProxy = lutPropertyHelper.GetAsProxy();
  vtkNew<vtkSMTransferFunctionManager> mgr;
  vtkSMProxy* sbProxy = mgr->GetScalarBarRepresentation(lutProxy, view);
  if (!sbProxy)
  {
    vtkGenericWarningMacro("Failed to locate/create ScalarBar representation.");
    return -1;
  }

  sbProxy->UpdatePropertyInformation();
  return vtkSMPropertyHelper(sbProxy, "EstimatedNumberOfAnnotations").GetAsInt();
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMColorMapEditorHelper::GetLastLUTProxy(vtkSMProxy* proxy)
{
  vtkSMPVRepresentationProxy* repr = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
  return repr ? repr->GetLastLUTProxy() : nullptr;
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetLastLUTProxy(vtkSMProxy* proxy, vtkSMProxy* lutProxy)
{
  vtkSMPVRepresentationProxy* repr = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
  if (repr)
  {
    repr->SetLastLUTProxy(lutProxy);
  }
}
