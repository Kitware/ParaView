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
#include "vtkSMProxyProperty.h"
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
    const vtkSMPropertyHelper helper(proxy->GetProperty("ColorArrayName"));
    return (helper.GetNumberOfElements() == 5 && helper.GetAsString(4) &&
      strcmp(helper.GetAsString(4), "") != 0);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::GetBlockUsingScalarColoring(
  vtkSMProxy* proxy, const std::string& blockSelector)
{
  if (!vtkSMRepresentationProxy::SafeDownCast(proxy))
  {
    return false;
  }

  const auto blockArrayAttributeTypeAndName =
    vtkSMColorMapEditorHelper::GetBlockColorArray(proxy, blockSelector);
  return blockArrayAttributeTypeAndName.first != -1 &&
    !blockArrayAttributeTypeAndName.second.empty();
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::GetAnyBlockUsingScalarColoring(vtkSMProxy* proxy)
{
  if (!vtkSMRepresentationProxy::SafeDownCast(proxy))
  {
    return false;
  }
  auto blockColorArray =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockColorArrayNames"));
  if (!blockColorArray)
  {
    return false;
  }
  for (unsigned int i = 0; i < blockColorArray->GetNumberOfElements(); i += 3)
  {
    if (vtkSMColorMapEditorHelper::GetBlockUsingScalarColoring(
          proxy, blockColorArray->GetElement(i)))
    {
      return true;
    }
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
    const vtkSMPropertyHelper helper(proxy, "ColorArrayName");
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
        const int rescaleMode =
          vtkSMPropertyHelper(lutProxy, "AutomaticRescaleRangeMode", true).GetAsInt();
        vtkSMPropertyHelper(lutProperty).Set(lutProxy);
        const bool extend = rescaleMode == vtkSMTransferFunctionManager::GROW_ON_APPLY;
        const bool force = false;
        vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(proxy, extend, force);
        proxy->UpdateVTKObjects();
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetupBlockLookupTable(
  vtkSMProxy* proxy, const std::string& blockSelector)
{
  if (vtkSMColorMapEditorHelper::GetBlockUsingScalarColoring(proxy, blockSelector))
  {
    // If representation has been initialized to use scalar coloring and no
    // transfer functions are setup, we setup the transfer functions.
    const auto repr = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    const auto blockArrayAttributeTypeAndName =
      vtkSMColorMapEditorHelper::GetBlockColorArray(repr, blockSelector);
    const auto arrayName = blockArrayAttributeTypeAndName.second;
    if (!arrayName.empty())
    {
      vtkNew<vtkSMTransferFunctionManager> mgr;
      const bool hasBlockLUT =
        repr->GetProperty("BlockLookupTableSelectors") && repr->GetProperty("BlockLookupTables");
      if (hasBlockLUT)
      {
        auto lutProxy = vtkSMTransferFunctionProxy::SafeDownCast(
          mgr->GetColorTransferFunction(arrayName.c_str(), proxy->GetSessionProxyManager()));
        const int rescaleMode =
          vtkSMPropertyHelper(lutProxy, "AutomaticRescaleRangeMode", true).GetAsInt();
        vtkSMColorMapEditorHelper::SetBlockLookupTable(proxy, blockSelector, lutProxy);
        const bool extend = rescaleMode == vtkSMTransferFunctionManager::GROW_ON_APPLY;
        const bool force = false;
        vtkSMColorMapEditorHelper::RescaleBlockTransferFunctionToDataRange(
          proxy, blockSelector, extend, force);
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
  return vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(
    proxy, vtkSMColorMapEditorHelper::GetArrayInformationForColorArray(proxy), extend, force);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(
  vtkSMProxy* proxy, const char* arrayName, int attributeType, bool extend, bool force)
{
  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  if (!repr)
  {
    return false;
  }

  const vtkSMPropertyHelper inputHelper(proxy->GetProperty("Input"));
  vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  const int port = inputHelper.GetOutputPort();
  if (!inputProxy)
  {
    // no input.
    vtkGenericWarningMacro("No input present. Cannot determine data ranges.");
    return false;
  }

  vtkPVDataInformation* dataInfo = inputProxy->GetDataInformation(port);
  vtkPVArrayInformation* info = dataInfo->GetArrayInformation(arrayName, attributeType);
  if (!info)
  {
    vtkPVDataInformation* representedDataInfo = repr->GetRepresentedDataInformation();
    info = representedDataInfo->GetArrayInformation(arrayName, attributeType);
  }

  return vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(proxy, info, extend, force);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::RescaleBlockTransferFunctionToDataRange(
  vtkSMProxy* proxy, const std::string& blockSelector, bool extend, bool force)
{
  if (!vtkSMColorMapEditorHelper::GetBlockUsingScalarColoring(proxy, blockSelector))
  {
    // we are not using scalar coloring, nothing to do.
    return false;
  }

  SM_SCOPED_TRACE(CallMethod)
    .arg(proxy)
    .arg("RescaleBlockTransferFunctionToDataRange")
    .arg(blockSelector.c_str())
    .arg(extend)
    .arg(force)
    .arg("comment",
      (extend
          ? "rescale block color and/or opacity maps used to include current data range"
          : "rescale block color and/or opacity maps used to exactly fit the current data range"));
  return vtkSMColorMapEditorHelper::RescaleBlockTransferFunctionToDataRange(proxy, blockSelector,
    vtkSMColorMapEditorHelper::GetBlockArrayInformationForColorArray(proxy, blockSelector), extend,
    force);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::RescaleBlockTransferFunctionToDataRange(vtkSMProxy* proxy,
  const std::string& blockSelector, const char* arrayName, int attributeType, bool extend,
  bool force)
{
  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  const vtkSMPropertyHelper inputHelper(proxy->GetProperty("Input"));
  vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  const int port = inputHelper.GetOutputPort();
  if (!inputProxy || !inputProxy->GetOutputPort(port))
  {
    // no input.
    vtkWarningWithObjectMacro(proxy, "No input present. Cannot determine data ranges.");
    return false;
  }

  vtkPVDataInformation* blockDataInfo = inputProxy->GetOutputPort(port)->GetSubsetDataInformation(
    blockSelector.c_str(), vtkSMPropertyHelper(proxy, "Assembly", true).GetAsString());
  vtkPVArrayInformation* blockArrayInfo =
    blockDataInfo->GetArrayInformation(arrayName, attributeType);
  if (!blockArrayInfo)
  {
    vtkPVDataInformation* representedDataInfo = repr->GetRepresentedDataInformation();
    blockArrayInfo = representedDataInfo->GetArrayInformation(arrayName, attributeType);
  }

  return vtkSMColorMapEditorHelper::RescaleBlockTransferFunctionToDataRange(
    repr, blockSelector, blockArrayInfo, extend, force);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRangeOverTime(vtkSMProxy* proxy)
{
  const vtkSMPropertyHelper helper(proxy->GetProperty("ColorArrayName"));

  return vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRangeOverTime(
    proxy, helper.GetAsString(4), helper.GetAsInt(3));
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRangeOverTime(
  vtkSMProxy* proxy, const char* arrayName, int attributeType)
{
  const vtkSMPropertyHelper inputHelper(proxy->GetProperty("Input"));
  vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  const int port = inputHelper.GetOutputPort();
  if (!inputProxy || !inputProxy->GetOutputPort(port))
  {
    // no input.
    vtkGenericWarningMacro("No input present. Cannot determine data ranges.");
    return false;
  }

  vtkPVTemporalDataInformation* dataInfo =
    inputProxy->GetOutputPort(port)->GetTemporalDataInformation();
  vtkPVArrayInformation* info = dataInfo->GetArrayInformation(arrayName, attributeType);
  return info ? RescaleTransferFunctionToDataRange(proxy, info) : false;
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::RescaleBlockTransferFunctionToDataRangeOverTime(
  vtkSMProxy* proxy, const std::string& blockSelector)
{
  if (!vtkSMColorMapEditorHelper::GetBlockUsingScalarColoring(proxy, blockSelector))
  {
    // we are not using scalar coloring, nothing to do.
    return false;
  }

  const auto blockArrayAttributeTypeAndName =
    vtkSMColorMapEditorHelper::GetBlockColorArray(proxy, blockSelector);
  return vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRangeOverTime(
    proxy, blockArrayAttributeTypeAndName.second.c_str(), blockArrayAttributeTypeAndName.first);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::RescaleBlockTransferFunctionToDataRangeOverTime(
  vtkSMProxy* proxy, const std::string& blockSelector, const char* arrayName, int attributeType)
{
  const vtkSMPropertyHelper inputHelper(proxy->GetProperty("Input"));
  auto inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  const int port = inputHelper.GetOutputPort();
  if (!inputProxy || !inputProxy->GetOutputPort(port))
  {
    // no input.
    vtkWarningWithObjectMacro(proxy, "No input present. Cannot determine data ranges.");
    return false;
  }

  vtkPVTemporalDataInformation* blockDataInfo =
    inputProxy->GetOutputPort(port)->GetTemporalSubsetDataInformation(
      blockSelector.c_str(), vtkSMPropertyHelper(proxy, "Assembly", true).GetAsString());
  vtkPVArrayInformation* blockArrayInfo =
    blockDataInfo->GetArrayInformation(arrayName, attributeType);
  return blockArrayInfo ? vtkSMColorMapEditorHelper::RescaleBlockTransferFunctionToDataRange(
                            proxy, blockSelector, blockArrayInfo)
                        : false;
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(
  vtkSMProxy* proxy, vtkPVArrayInformation* info, bool extend, bool force)
{
  if (!vtkSMColorMapEditorHelper::GetUsingScalarColoring(proxy))
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
    const int indexedLookup = vtkSMPropertyHelper(lut, "IndexedLookup").GetAsInt();
    if (indexedLookup > 0)
    {
      vtkPVProminentValuesInformation* prominentValues =
        vtkSMColorMapEditorHelper::GetProminentValuesInformationForColorArray(proxy);
      auto activeAnnotations = vtkSmartPointer<vtkStringList>::New();
      auto activeIndexedColors = vtkSmartPointer<vtkDoubleArray>::New();
      vtkSmartPointer<vtkAbstractArray> uniqueValues;

      uniqueValues.TakeReference(prominentValues->GetProminentComponentValues(component));

      vtkSMStringVectorProperty* allAnnotations =
        vtkSMStringVectorProperty::SafeDownCast(lut->GetProperty("Annotations"));
      vtkSMStringVectorProperty* activeAnnotatedValuesProperty =
        vtkSMStringVectorProperty::SafeDownCast(lut->GetProperty("ActiveAnnotatedValues"));
      if (uniqueValues && allAnnotations && activeAnnotatedValuesProperty)
      {
        auto activeAnnotatedValues = vtkSmartPointer<vtkStringList>::New();
        if (extend)
        {
          activeAnnotatedValuesProperty->GetElements(activeAnnotatedValues);
        }

        for (int idx = 0; idx < uniqueValues->GetNumberOfTuples(); ++idx)
        {
          // Look up index of color corresponding to the annotation
          for (unsigned int j = 0; j < allAnnotations->GetNumberOfElements() / 2; ++j)
          {
            const vtkVariant annotatedValue(allAnnotations->GetElement(2 * j + 0));
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
        const vtkSMPropertyHelper inputHelper(proxy->GetProperty("Input"));
        vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
        const int port = inputHelper.GetOutputPort();
        if (!inputProxy)
        {
          // no input.
          vtkGenericWarningMacro("No input present. Cannot determine opacity data range.");
          return false;
        }

        vtkPVDataInformation* dataInfo = inputProxy->GetDataInformation(port);
        const vtkSMPropertyHelper opacityArrayNameHelper(proxy, "OpacityArrayName");
        const int opacityArrayFieldAssociation = opacityArrayNameHelper.GetAsInt(3);
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
bool vtkSMColorMapEditorHelper::RescaleBlockTransferFunctionToDataRange(vtkSMProxy* proxy,
  const std::string& blockSelector, vtkPVArrayInformation* info, bool extend, bool force)
{
  if (!info)
  {
    vtkGenericWarningMacro("Could not determine array range.");
    return false;
  }

  vtkSMProxy* blockLut = vtkSMColorMapEditorHelper::GetBlockLookupTable(proxy, blockSelector);
  if (!blockLut)
  {
    return false;
  }

  if (force == false &&
    vtkSMPropertyHelper(blockLut, "AutomaticRescaleRangeMode", true).GetAsInt() ==
      vtkSMTransferFunctionManager::NEVER)
  {
    // nothing to change, range is locked.
    return true;
  }

  // We need to determine the component number to use from the lut.
  int component = -1;
  if (blockLut && vtkSMPropertyHelper(blockLut, "VectorMode").GetAsInt() != 0)
  {
    component = vtkSMPropertyHelper(blockLut, "VectorComponent").GetAsInt();
  }

  if (blockLut && component < info->GetNumberOfComponents())
  {
    const int indexedLookup = vtkSMPropertyHelper(blockLut, "IndexedLookup").GetAsInt();
    if (indexedLookup)
    {
      auto prominentValues =
        vtkSMColorMapEditorHelper::GetBlockProminentValuesInformationForColorArray(
          proxy, blockSelector);
      auto activeAnnotations = vtkSmartPointer<vtkStringList>::New();
      auto activeIndexedColors = vtkSmartPointer<vtkDoubleArray>::New();
      vtkSmartPointer<vtkAbstractArray> uniqueValues;

      uniqueValues.TakeReference(prominentValues->GetProminentComponentValues(component));

      auto allAnnotations =
        vtkSMStringVectorProperty::SafeDownCast(blockLut->GetProperty("Annotations"));
      auto activeAnnotatedValuesProperty =
        vtkSMStringVectorProperty::SafeDownCast(blockLut->GetProperty("ActiveAnnotatedValues"));
      if (uniqueValues && allAnnotations && activeAnnotatedValuesProperty)
      {
        auto activeAnnotatedValues = vtkSmartPointer<vtkStringList>::New();

        if (extend)
        {
          activeAnnotatedValuesProperty->GetElements(activeAnnotatedValues);
        }

        for (int idx = 0; idx < uniqueValues->GetNumberOfTuples(); ++idx)
        {
          // Look up index of color corresponding to the annotation
          for (unsigned int j = 0; j < allAnnotations->GetNumberOfElements() / 2; ++j)
          {
            const vtkVariant annotatedValue(allAnnotations->GetElement(2 * j + 0));
            if (annotatedValue == uniqueValues->GetVariantValue(idx))
            {
              activeAnnotatedValues->AddString(allAnnotations->GetElement(2 * j + 0));
              break;
            }
          }
        }

        activeAnnotatedValuesProperty->SetElements(activeAnnotatedValues);
        blockLut->UpdateVTKObjects();
      }
    }
    else
    {
      double rangeColor[2];
      double rangeOpacity[2];

      // No opacity array for block coloring
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

      if (blockLut && rangeColor[1] >= rangeColor[0])
      {
        vtkSMTransferFunctionProxy::RescaleTransferFunction(blockLut, rangeColor, extend);
        vtkSMProxy* sof_lut =
          vtkSMPropertyHelper(blockLut, "ScalarOpacityFunction", true).GetAsProxy();
        if (sof_lut && rangeOpacity[1] >= rangeOpacity[0])
        {
          vtkSMTransferFunctionProxy::RescaleTransferFunction(sof_lut, rangeOpacity, extend);
        }
      }
      return blockLut;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::RescaleTransferFunctionToVisibleRange(
  vtkSMProxy* proxy, vtkSMProxy* view)
{
  const vtkSMPropertyHelper helper(proxy->GetProperty("ColorArrayName"));
  return vtkSMColorMapEditorHelper::RescaleTransferFunctionToVisibleRange(
    proxy, view, helper.GetAsString(4), helper.GetAsInt(3));
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::RescaleTransferFunctionToVisibleRange(
  vtkSMProxy* proxy, vtkSMProxy* view, const char* arrayName, int attributeType)
{
  if (!vtkSMColorMapEditorHelper::GetUsingScalarColoring(proxy))
  {
    // we are not using scalar coloring, nothing to do.
    return false;
  }

  // TODO: Add option for charts
  vtkSMRenderViewProxy* rview = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!rview || !arrayName || arrayName[0] == '\0')
  {
    return false;
  }

  const vtkSMPropertyHelper inputHelper(proxy->GetProperty("Input"));
  auto inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  const int port = inputHelper.GetOutputPort();
  if (!inputProxy || !inputProxy->GetOutputPort(port))
  {
    // no input.
    vtkGenericWarningMacro("No input present. Cannot determine data ranges.");
    return false;
  }

  vtkPVDataInformation* dataInfo = inputProxy->GetOutputPort(port)->GetDataInformation();
  vtkPVArrayInformation* info = dataInfo->GetArrayInformation(arrayName, attributeType);
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
  const bool useTransfer2D =
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
  if (!rview->ComputeVisibleScalarRange(attributeType, arrayName, component, range))
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
bool vtkSMColorMapEditorHelper::RescaleBlockTransferFunctionToVisibleRange(
  vtkSMProxy* proxy, vtkSMProxy* view, const std::string& blockSelector)
{
  if (!vtkSMColorMapEditorHelper::GetBlockUsingScalarColoring(proxy, blockSelector))
  {
    // we are not using scalar coloring, nothing to do.
    return false;
  }

  const auto blockArrayAttributeTypeAndName =
    vtkSMColorMapEditorHelper::GetBlockColorArray(proxy, blockSelector);
  return vtkSMColorMapEditorHelper::RescaleBlockTransferFunctionToVisibleRange(proxy, view,
    blockSelector, blockArrayAttributeTypeAndName.second.c_str(),
    blockArrayAttributeTypeAndName.first);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::RescaleBlockTransferFunctionToVisibleRange(vtkSMProxy* proxy,
  vtkSMProxy* view, const std::string& blockSelector, const char* arrayName, int attributeType)
{
  auto rview = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!rview || !arrayName || arrayName[0] == 0)
  {
    return false;
  }

  const vtkSMPropertyHelper helper(proxy->GetProperty("Input"));
  vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(helper.GetAsProxy());
  const int port = helper.GetOutputPort();
  if (!inputProxy || !inputProxy->GetOutputPort(port))
  {
    vtkGenericWarningMacro("No input present. Cannot determine data ranges.");
    return false;
  }

  auto blockDataInfo = inputProxy->GetOutputPort(port)->GetSubsetDataInformation(
    blockSelector.c_str(), vtkSMPropertyHelper(proxy->GetProperty("Assembly"), true).GetAsString());
  vtkPVArrayInformation* info = blockDataInfo->GetArrayInformation(arrayName, attributeType);
  if (!info)
  {
    return false;
  }

  const bool hasBlockLUT =
    proxy->GetProperty("BlockLookupTableSelectors") && proxy->GetProperty("BlockLookupTables");
  if (!hasBlockLUT)
  {
    // No LookupTable found.
    return false;
  }

  vtkSMProxy* blockLUT = vtkSMColorMapEditorHelper::GetBlockLookupTable(proxy, blockSelector);

  // We need to determine the component number to use from the lut.
  int component = -1;
  if (blockLUT && vtkSMPropertyHelper(blockLUT, "VectorMode").GetAsInt() != 0)
  {
    component = vtkSMPropertyHelper(blockLUT, "VectorComponent").GetAsInt();
  }
  if (component >= info->GetNumberOfComponents())
  {
    // something amiss, the component request is not present in the dataset.
    // give up.
    return false;
  }

  double range[2];
  if (!rview->ComputeVisibleScalarRange(attributeType, arrayName, component, range))
  {
    return false;
  }

  if (blockLUT)
  {
    vtkSMTransferFunctionProxy::RescaleTransferFunction(blockLUT, range, false);
    vtkSMProxy* sof_lut = vtkSMPropertyHelper(blockLUT, "ScalarOpacityFunction", true).GetAsProxy();
    if (sof_lut)
    {
      vtkSMTransferFunctionProxy::RescaleTransferFunction(sof_lut, range, false);
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::SetScalarColoring(
  vtkSMProxy* proxy, const char* arrayName, int attributeType)
{
  return vtkSMColorMapEditorHelper::SetScalarColoringInternal(
    proxy, arrayName, attributeType, false, -1);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::SetScalarColoring(
  vtkSMProxy* proxy, const char* arrayName, int attributeType, int component)
{
  return vtkSMColorMapEditorHelper::SetScalarColoringInternal(
    proxy, arrayName, attributeType, true, component);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::SetScalarColoringInternal(
  vtkSMProxy* proxy, const char* arrayName, int attributeType, bool useComponent, int component)
{
  if (!vtkSMColorMapEditorHelper::GetUsingScalarColoring(proxy) &&
    (!arrayName || arrayName[0] == '\0'))
  {
    // if true, scalar coloring already off: Nothing to do.
    return true;
  }

  vtkSMProperty* colorArray = proxy->GetProperty("ColorArrayName");
  vtkSMPropertyHelper colorArrayHelper(colorArray);
  colorArrayHelper.SetInputArrayToProcess(attributeType, arrayName);

  if (!arrayName || arrayName[0] == '\0')
  {
    SM_SCOPED_TRACE(SetScalarColoring)
      .arg("display", proxy)
      .arg("arrayname", arrayName)
      .arg("attribute_type", attributeType);
    vtkSMPropertyHelper(proxy, "LookupTable", true).RemoveAllValues();
    vtkSMPropertyHelper(proxy, "ScalarOpacityFunction", true).RemoveAllValues();
    proxy->UpdateVTKObjects();
    return true;
  }

  auto* arraySettings = vtkPVRepresentedArrayListSettings::GetInstance();
  auto* arrayInfo = vtkSMColorMapEditorHelper::GetArrayInformationForColorArray(proxy, false);
  const bool forceComponentMode = (arrayInfo && arraySettings &&
    !arraySettings->ShouldUseMagnitudeMode(arrayInfo->GetNumberOfComponents()));
  if (forceComponentMode && (!useComponent || component < 0))
  {
    component = 0;
  }

  // Now, setup transfer functions.
  bool haveComponent = useComponent;
  const bool separate = (vtkSMPropertyHelper(proxy, "UseSeparateColorMap", true).GetAsInt() != 0);
  const bool useTransfer2D = (vtkSMPropertyHelper(proxy, "UseTransfer2D", true).GetAsInt() != 0);
  const std::string decoratedArrayName =
    vtkSMColorMapEditorHelper::GetDecoratedArrayName(proxy, arrayName);
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
    vtkPVArrayInformation* colorArrayInfo =
      vtkSMColorMapEditorHelper::GetArrayInformationForColorArray(proxy);
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
            .arg("arrayname", arrayName)
            .arg("attribute_type", attributeType)
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
      .arg("arrayname", arrayName)
      .arg("attribute_type", attributeType)
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
bool vtkSMColorMapEditorHelper::SetBlockScalarColoring(
  vtkSMProxy* proxy, const std::string& blockSelector, const char* arrayName, int attributeType)
{
  return vtkSMColorMapEditorHelper::SetBlockScalarColoringInternal(
    proxy, blockSelector, arrayName, attributeType, false, -1);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::SetBlockScalarColoring(vtkSMProxy* proxy,
  const std::string& blockSelector, const char* arrayName, int attributeType, int component)
{
  return vtkSMColorMapEditorHelper::SetBlockScalarColoringInternal(
    proxy, blockSelector, arrayName, attributeType, true, component);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::SetBlockScalarColoringInternal(vtkSMProxy* proxy,
  const std::string& blockSelector, const char* arrayName, int attributeType, bool useComponent,
  int component)
{
  auto repr = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
  if (!repr)
  {
    return false;
  }

  if (!vtkSMColorMapEditorHelper::GetBlockUsingScalarColoring(proxy, blockSelector) &&
    (arrayName == nullptr || arrayName[0] == 0))
  {
    // scalar coloring already off. Nothing to do.
    return true;
  }
  auto blockColorArray = repr->GetProperty("BlockColorArrayNames");
  if (!blockColorArray)
  {
    vtkWarningWithObjectMacro(repr, "No 'BlockColorArrayNames' property found.");
    return false;
  }

  vtkSMColorMapEditorHelper::SetBlockColorArray(
    repr, blockSelector, attributeType, arrayName ? arrayName : "");

  if (arrayName == nullptr || arrayName[0] == '\0')
  {
    SM_SCOPED_TRACE(SetBlockScalarColoring)
      .arg("display", repr)
      .arg("block_selector", blockSelector.c_str())
      .arg("arrayname", arrayName)
      .arg("attribute_type", attributeType);

    vtkSMColorMapEditorHelper::RemoveBlockLookupTable(repr, blockSelector);
    // Scalar Opacity is not supported per block yet
    repr->UpdateVTKObjects();
    return true;
  }

  auto* arraySettings = vtkPVRepresentedArrayListSettings::GetInstance();
  auto blockArrayInfo =
    vtkSMColorMapEditorHelper::GetBlockArrayInformationForColorArray(repr, blockSelector, false);
  const bool forceComponentMode = (blockArrayInfo && arraySettings &&
    !arraySettings->ShouldUseMagnitudeMode(blockArrayInfo->GetNumberOfComponents()));
  if (forceComponentMode && (!useComponent || component < 0))
  {
    component = 0;
  }

  // Now, setup transfer functions.
  bool haveComponent = useComponent;
  const bool separate = vtkSMColorMapEditorHelper::GetBlockUseSeparateColorMap(repr, blockSelector);
  const std::string decoratedArrayName =
    vtkSMColorMapEditorHelper::GetBlockDecoratedArrayName(repr, blockSelector, arrayName);
  const bool hasBlockLUTProperty = repr->GetProperty("BlockLookupTableSelectors") != nullptr &&
    repr->GetProperty("BlockLookupTables") != nullptr;
  vtkNew<vtkSMTransferFunctionManager> mgr;
  vtkSMProxy* lutProxy = nullptr;
  if (hasBlockLUTProperty)
  {
    lutProxy =
      mgr->GetColorTransferFunction(decoratedArrayName.c_str(), repr->GetSessionProxyManager());
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

    vtkSMColorMapEditorHelper::SetBlockLookupTable(repr, blockSelector, lutProxy);

    // Get the array information for the color array to determine transfer function properties
    auto blockColorArrayInfo =
      vtkSMColorMapEditorHelper::GetBlockArrayInformationForColorArray(repr, blockSelector, true);
    if (blockColorArrayInfo)
    {
      if (blockColorArrayInfo->GetDataType() == VTK_STRING)
      {
        vtkSMPropertyHelper(lutProxy, "IndexedLookup", true).Set(1);
        lutProxy->UpdateVTKObjects();
      }
      if (haveComponent)
      {
        const char* componentName = blockColorArrayInfo->GetComponentName(component);
        if (strcmp(componentName, "") != 0)
        {
          SM_SCOPED_TRACE(SetBlockScalarColoring)
            .arg("display", repr)
            .arg("block_selector", blockSelector.c_str())
            .arg("arrayname", arrayName)
            .arg("attribute_type", attributeType)
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
    SM_SCOPED_TRACE(SetBlockScalarColoring)
      .arg("display", repr)
      .arg("block_selector", blockSelector.c_str())
      .arg("arrayname", arrayName)
      .arg("attribute_type", attributeType)
      .arg("separate", separate);
  }

  repr->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
std::string vtkSMColorMapEditorHelper::GetDecoratedArrayName(
  vtkSMProxy* proxy, const std::string& arrayName)
{
  std::ostringstream ss;
  ss << arrayName;
  if (proxy->GetProperty("TransferFunction2D"))
  {
    const bool useGradientAsY =
      (vtkSMPropertyHelper(proxy, "UseGradientForTransfer2D", true).GetAsInt() == 1);
    if (!useGradientAsY)
    {
      std::string array2Name;
      vtkSMProperty* colorArray2Property = proxy->GetProperty("ColorArray2Name");
      if (colorArray2Property)
      {
        const vtkSMPropertyHelper colorArray2Helper(colorArray2Property);
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
std::string vtkSMColorMapEditorHelper::GetBlockDecoratedArrayName(
  vtkSMProxy* proxy, const std::string& blockSelector, const std::string& arrayName)
{
  std::ostringstream ss;
  ss << arrayName;
  if (vtkSMColorMapEditorHelper::GetBlockUseSeparateColorMap(proxy, blockSelector))
  {
    std::ostringstream ss1;
    ss1 << "Separate_" << proxy->GetGlobalIDAsString() << "_" << blockSelector << "_" << ss.str();
    return ss1.str();
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
void vtkSMColorMapEditorHelper::SetBlockColorArray(
  vtkSMProxy* proxy, const std::string& blockSelector, int attributeType, std::string arrayName)
{
  auto blockColorArray =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockColorArrayNames"));
  if (!blockColorArray)
  {
    vtkWarningWithObjectMacro(proxy, "No 'BlockColorArrayNames' property found.");
    return;
  }
  assert(blockColorArray->GetNumberOfElementsPerCommand() == 3);
  if (arrayName.empty())
  {
    // find the block selector
    bool found = false;
    unsigned selectorIdx = 0;
    for (unsigned int i = 0; i < blockColorArray->GetNumberOfElements(); i += 3)
    {
      if (blockColorArray->GetElement(i) == blockSelector)
      {
        // found the block selector
        selectorIdx = i;
        found = true;
        break;
      }
    }
    if (found)
    {
      std::vector<std::string> blockColorArrayVec;
      // remove the block selector and color array
      for (unsigned int i = 0; i < blockColorArray->GetNumberOfElements(); i += 3)
      {
        if (i != selectorIdx)
        {
          blockColorArrayVec.push_back(blockColorArray->GetElement(i));
          blockColorArrayVec.push_back(blockColorArray->GetElement(i + 1));
          blockColorArrayVec.push_back(blockColorArray->GetElement(i + 2));
        }
      }
      blockColorArray->SetElements(blockColorArrayVec);
    }
  }
  else
  {
    bool found = false;
    for (unsigned int i = 0; i < blockColorArray->GetNumberOfElements(); i += 3)
    {
      if (blockColorArray->GetElement(i) == blockSelector)
      {
        // found the block selector, update the color array
        blockColorArray->SetElement(i + 1, std::to_string(attributeType).c_str());
        blockColorArray->SetElement(i + 2, arrayName.c_str());
        found = true;
        break;
      }
    }
    if (!found)
    {
      // add the block selector and color array
      blockColorArray->AppendElements({ blockSelector, std::to_string(attributeType), arrayName });
    }
  }
}

//----------------------------------------------------------------------------
std::pair<int, std::string> vtkSMColorMapEditorHelper::GetBlockColorArray(
  vtkSMProxy* proxy, const std::string& blockSelector)
{
  auto blockColorArray =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockColorArrayNames"));
  if (!blockColorArray)
  {
    vtkWarningWithObjectMacro(proxy, "No 'BlockColorArrayNames' property found.");
    return std::make_pair(-1, std::string(""));
  }
  assert(blockColorArray->GetNumberOfElementsPerCommand() == 3);
  for (unsigned int i = 0; i < blockColorArray->GetNumberOfElements(); i += 3)
  {
    if (blockColorArray->GetElement(i) == blockSelector)
    {
      return std::make_pair(std::stoi(blockColorArray->GetElement(i + 1)),
        std::string(blockColorArray->GetElement(i + 2)));
    }
  }
  return std::make_pair(-1, std::string(""));
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetBlockUseSeparateColorMap(
  vtkSMProxy* proxy, const std::string& blockSelector, bool use)
{
  auto blockUseSeparateColorMaps =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockUseSeparateColorMaps"));
  if (!blockUseSeparateColorMaps)
  {
    vtkWarningWithObjectMacro(proxy, "No 'blockUseSeparateColorMaps' property found.");
    return;
  }
  assert(blockUseSeparateColorMaps->GetNumberOfElementsPerCommand() == 2);
  bool found = false;
  for (unsigned int i = 0; i < blockUseSeparateColorMaps->GetNumberOfElements(); i += 3)
  {
    if (blockUseSeparateColorMaps->GetElement(i) == blockSelector)
    {
      // found the block selector, update the color array
      blockUseSeparateColorMaps->SetElement(i + 1, std::to_string(use).c_str());
      found = true;
      break;
    }
  }
  if (!found)
  {
    // add the block selector and color array
    blockUseSeparateColorMaps->AppendElements({ blockSelector, std::to_string(use) });
  }
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::GetBlockUseSeparateColorMap(
  vtkSMProxy* proxy, const std::string& blockSelector)
{
  auto blockUseSeparateColorMaps =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockUseSeparateColorMaps"));
  if (!blockUseSeparateColorMaps)
  {
    vtkWarningWithObjectMacro(proxy, "No 'blockUseSeparateColorMaps' property found.");
    return false;
  }
  assert(blockUseSeparateColorMaps->GetNumberOfElementsPerCommand() == 2);
  for (unsigned int i = 0; i < blockUseSeparateColorMaps->GetNumberOfElements(); i += 2)
  {
    if (blockUseSeparateColorMaps->GetElement(i) == blockSelector)
    {
      if (std::stoi(blockUseSeparateColorMaps->GetElement(i + 1)) == 1)
      {
        return true;
      }
    }
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetBlockLookupTable(
  vtkSMProxy* proxy, const std::string& blockSelector, vtkSMProxy* lutProxy)
{
  auto blockLUTSelectors =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockLookupTableSelectors"));
  auto blockLUTs = vtkSMProxyProperty::SafeDownCast(proxy->GetProperty("BlockLookupTables"));
  if (!blockLUTSelectors || !blockLUTs)
  {
    vtkWarningWithObjectMacro(
      proxy, "Missing 'BlockLookupTableSelectors' or 'BlockLookupTables' property.");
    return;
  }
  bool found = false;
  for (unsigned int i = 0; i < blockLUTSelectors->GetNumberOfElements(); ++i)
  {
    if (blockLUTSelectors->GetElement(i) == blockSelector)
    {
      blockLUTs->SetProxy(i, lutProxy);
      found = true;
    }
  }
  if (!found)
  {
    blockLUTSelectors->AppendElements({ blockSelector });
    blockLUTs->AddProxy(lutProxy);
  }
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMColorMapEditorHelper::GetBlockLookupTable(
  vtkSMProxy* proxy, const std::string& blockSelector)
{
  auto blockLUTSelectors =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockLookupTableSelectors"));
  auto blockLUTs = vtkSMProxyProperty::SafeDownCast(proxy->GetProperty("BlockLookupTables"));
  if (!blockLUTSelectors || !blockLUTs)
  {
    vtkWarningWithObjectMacro(
      proxy, "Missing 'BlockLookupTableSelectors' or 'BlockLookupTables' property.");
    return nullptr;
  }
  for (unsigned int i = 0; i < blockLUTSelectors->GetNumberOfElements(); ++i)
  {
    if (blockLUTSelectors->GetElement(i) == blockSelector)
    {
      return blockLUTs->GetProxy(i);
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::RemoveBlockLookupTable(
  vtkSMProxy* proxy, const std::string& blockSelector)
{
  auto blockLUTSelectors =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockLookupTableSelectors"));
  auto blockLUTs = vtkSMProxyProperty::SafeDownCast(proxy->GetProperty("BlockLookupTables"));
  if (!blockLUTSelectors || !blockLUTs)
  {
    vtkWarningWithObjectMacro(
      proxy, "Missing 'BlockLookupTableSelectors' or 'BlockLookupTables' property.");
    return;
  }
  // find selector (if any) and remove the selector and the lut
  bool found = false;
  unsigned int selectorIdx = 0;
  for (unsigned int i = 0; i < blockLUTSelectors->GetNumberOfElements(); ++i)
  {
    if (blockLUTSelectors->GetElement(i) == blockSelector)
    {
      selectorIdx = i;
      found = true;
      break;
    }
  }
  if (found)
  {
    std::vector<std::string> blockLUTSelectorsVec;
    std::vector<vtkSMProxy*> blockLUTsVec;
    for (unsigned int i = 0; i < blockLUTSelectors->GetNumberOfElements(); ++i)
    {
      if (i != selectorIdx)
      {
        blockLUTSelectorsVec.push_back(blockLUTSelectors->GetElement(i));
        blockLUTsVec.push_back(blockLUTs->GetProxy(i));
      }
    }
    blockLUTSelectors->SetElements(blockLUTSelectorsVec);
    blockLUTs->SetProxies(static_cast<unsigned int>(blockLUTsVec.size()), blockLUTsVec.data());
  }
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
  const vtkSMPropertyHelper lutPropertyHelper(lutProperty);
  if (lutPropertyHelper.GetNumberOfElements() == 0 || !lutPropertyHelper.GetAsProxy())
  {
    vtkGenericWarningMacro("Failed to determine the LookupTable being used.");
    return -1;
  }
  vtkSMProxy* lutProxy = lutPropertyHelper.GetAsProxy();
  auto sbProxy = vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
    vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lutProxy, view));
  if (sbProxy)
  {
    const vtkSMPropertyHelper sbsvPropertyHelper(sbProxy->GetProperty("StickyVisible"));
    return sbsvPropertyHelper.GetNumberOfElements()
      ? vtkSMPropertyHelper(sbProxy, "StickyVisible").GetAsInt()
      : -1;
  }
  return -1;
}

//----------------------------------------------------------------------------
int vtkSMColorMapEditorHelper::IsBlockScalarBarStickyVisible(
  vtkSMProxy* proxy, vtkSMProxy* view, const std::string& blockSelector)
{
  if (!view)
  {
    return -1;
  }
  vtkSMProxy* blockLUTProxy = vtkSMColorMapEditorHelper::GetBlockLookupTable(proxy, blockSelector);
  if (!blockLUTProxy)
  {
    return -1;
  }
  auto sbProxy = vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
    vtkSMTransferFunctionProxy::FindScalarBarRepresentation(blockLUTProxy, view));
  if (sbProxy)
  {
    const vtkSMPropertyHelper sbsvPropertyHelper(sbProxy->GetProperty("StickyVisible"));
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
  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  if (!repr)
  {
    return false;
  }

  const bool usingScalarBarColoring = vtkSMColorMapEditorHelper::GetUsingScalarColoring(proxy);

  vtkSMProperty* lutProperty = proxy->GetProperty("LookupTable");
  if (!lutProperty)
  {
    vtkGenericWarningMacro("Missing 'LookupTable' property");
    return false;
  }

  const vtkSMPropertyHelper lutPropertyHelper(lutProperty);
  vtkSMProxy* lutProxy = usingScalarBarColoring ? lutPropertyHelper.GetAsProxy()
                                                : vtkSMColorMapEditorHelper::GetLastLUTProxy(proxy);

  if (!lutProxy)
  {
    return false;
  }

  auto sbProxy = vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
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

  vtkSMColorMapEditorHelper::SetLastLUTProxy(proxy, usingScalarBarColoring ? lutProxy : nullptr);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::UpdateBlockScalarBarRange(
  vtkSMProxy* proxy, vtkSMProxy* view, const std::string& blockSelector, bool deleteRange)
{
  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  if (!repr)
  {
    return false;
  }

  const bool usingScalarBarColoring =
    vtkSMColorMapEditorHelper::GetBlockUsingScalarColoring(repr, blockSelector);

  vtkSMProxy* lutProxy = usingScalarBarColoring
    ? vtkSMColorMapEditorHelper::GetBlockLookupTable(repr, blockSelector)
    : vtkSMColorMapEditorHelper::GetLastBlockLUTProxy(repr, blockSelector);
  if (!lutProxy)
  {
    return false;
  }

  auto sbProxy = vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
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
    sbProxy->RemoveBlockRange(repr, blockSelector);

    // If we do not want to delete this range, then we update it with its potential new value.
    if (!deleteRange)
    {
      sbProxy->AddBlockRange(repr, blockSelector);
    }

    double updatedRange[2];
    sbProxy->GetRange(updatedRange);
    minRangePropHelper.Set(updatedRange[0]);
    maxRangePropHelper.Set(updatedRange[1]);
  }
  sbSMProxy->UpdateVTKObjects();

  vtkSMColorMapEditorHelper::SetLastBlockLUTProxy(
    repr, blockSelector, usingScalarBarColoring ? lutProxy : nullptr);
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

  const vtkSMPropertyHelper lutPropertyHelper(lutProperty);
  if (lutPropertyHelper.GetNumberOfElements() == 0)
  {
    return nullptr;
  }

  return lutPropertyHelper.GetAsProxy();
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMColorMapEditorHelper::GetBlockLUTProxy(
  vtkSMProxy* proxy, vtkSMProxy* view, const std::string& blockSelector)
{
  if (!view)
  {
    return nullptr;
  }
  return vtkSMColorMapEditorHelper::GetBlockLookupTable(proxy, blockSelector);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::SetScalarBarVisibility(
  vtkSMProxy* proxy, vtkSMProxy* view, bool visible)
{
  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
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
  vtkSMProxy* lastLUTProxy = vtkSMColorMapEditorHelper::GetLastLUTProxy(proxy);
  if (lastLUTProxy && lutProxy != lastLUTProxy)
  {
    if (auto lastSBProxy = vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
          vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lastLUTProxy, view)))
    {
      lastSBProxy->RemoveRange(repr);
    }
  }

  auto sbProxy = vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
    vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lutProxy, view));

  if (sbProxy)
  {
    const int legacyVisible = vtkSMPropertyHelper(sbProxy, "Visibility").GetAsInt();
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
    // show the scalarbar, whether it was previously hidden.
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

  if (!vtkSMColorMapEditorHelper::GetUsingScalarColoring(proxy))
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

    const vtkSMPropertyHelper colorArrayHelper(proxy, "ColorArrayName");
    const std::string arrayName(colorArrayHelper.GetInputArrayNameToProcess());

    // Look up array-specific title for scalar bar
    std::ostringstream prefix;
    prefix << ".array_lookup_tables"
           << "." << arrayName << ".Title";

    const std::string arrayTitle = settings->GetSettingAsString(prefix.str().c_str(), arrayName);

    vtkSMPropertyHelper titlePropHelper(titleProp);

    if (sbProxy && strcmp(titlePropHelper.GetAsString(), arrayTitle.c_str()) != 0)
    {
      sbProxy->ClearRange();
    }

    titlePropHelper.Set(arrayTitle.c_str());

    // now, determine a name for it if possible.
    vtkPVArrayInformation* arrayInfo =
      vtkSMColorMapEditorHelper::GetArrayInformationForColorArray(proxy);
    vtkSMScalarBarWidgetRepresentationProxy::UpdateComponentTitle(sbSMProxy, arrayInfo);
  }
  vtkSMScalarBarWidgetRepresentationProxy::PlaceInView(sbSMProxy, view);
  sbSMProxy->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::SetBlockScalarBarVisibility(
  vtkSMProxy* proxy, vtkSMProxy* view, const std::string& blockSelector, bool visible)
{
  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  if (!repr || !view)
  {
    return false;
  }

  vtkSMProxy* lutBlockProxy =
    vtkSMColorMapEditorHelper::GetBlockLUTProxy(repr, view, blockSelector);
  if (!lutBlockProxy)
  {
    return false;
  }

  SM_SCOPED_TRACE(CallMethod)
    .arg(repr)
    .arg("SetBlockScalarBarVisibility")
    .arg(view)
    .arg(blockSelector.c_str())
    .arg(visible)
    .arg("comment",
      visible ? "show block color bar/color legend" : "hide block color bar/color legend");

  vtkSMProxy* lastBlockLUTProxy =
    vtkSMColorMapEditorHelper::GetLastBlockLUTProxy(repr, blockSelector);

  // If the lut proxy changed, we need to remove ourself (representation proxy)
  // from the scalar bar widget that we used to be linked to.
  if (lastBlockLUTProxy && lutBlockProxy != lastBlockLUTProxy)
  {
    if (auto lastSBProxy = vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
          vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lastBlockLUTProxy, view)))
    {
      lastSBProxy->RemoveBlockRange(repr, blockSelector);
    }
  }

  auto sbProxy = vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
    vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lutBlockProxy, view));

  if (sbProxy)
  {
    const int legacyVisible = vtkSMPropertyHelper(sbProxy, "Visibility").GetAsInt();
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
    // show the scalarbar, whether it was previously hidden.
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

  if (!vtkSMColorMapEditorHelper::GetBlockUsingScalarColoring(repr, blockSelector))
  {
    return false;
  }

  vtkNew<vtkSMTransferFunctionManager> mgr;
  vtkSMProxy* sbSMProxy = mgr->GetScalarBarRepresentation(lutBlockProxy, view);
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

    const auto blockAttributeTypeAndName =
      vtkSMColorMapEditorHelper::GetBlockColorArray(repr, blockSelector);
    const std::string arrayName = blockAttributeTypeAndName.second;

    // Look up array-specific title for scalar bar
    std::ostringstream prefix;
    prefix << ".array_lookup_tables"
           << "." << arrayName << ".Title";

    const std::string arrayTitle = settings->GetSettingAsString(prefix.str().c_str(), arrayName);

    vtkSMPropertyHelper titlePropHelper(titleProp);

    if (sbProxy && strcmp(titlePropHelper.GetAsString(), arrayTitle.c_str()) != 0)
    {
      sbProxy->ClearRange();
    }

    titlePropHelper.Set(arrayTitle.c_str());

    // now, determine a name for it if possible.
    auto blockArrayInfo =
      vtkSMColorMapEditorHelper::GetBlockArrayInformationForColorArray(repr, blockSelector);
    vtkSMScalarBarWidgetRepresentationProxy::UpdateComponentTitle(sbSMProxy, blockArrayInfo);
  }
  vtkSMScalarBarWidgetRepresentationProxy::PlaceInView(sbSMProxy, view);
  sbSMProxy->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::HideScalarBarIfNotNeeded(vtkSMProxy* proxy, vtkSMProxy* view)
{
  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  vtkSMProperty* lutProperty = proxy->GetProperty("LookupTable");
  if (!repr || !lutProperty)
  {
    return false;
  }

  const vtkSMPropertyHelper lutPropertyHelper(lutProperty);
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
  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  vtkSMProperty* lutProperty = proxy->GetProperty("LookupTable");
  if (!repr || !lutProperty)
  {
    return false;
  }

  const vtkSMPropertyHelper lutPropertyHelper(lutProperty);
  if (lutPropertyHelper.GetNumberOfElements() == 0 || !lutPropertyHelper.GetAsProxy())
  {
    return false;
  }

  vtkSMProxy* lutProxy = lutPropertyHelper.GetAsProxy();
  return vtkSMTransferFunctionProxy::IsScalarBarVisible(lutProxy, view);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::IsBlockScalarBarVisible(
  vtkSMProxy* proxy, vtkSMProxy* view, const std::string& blockSelector)
{
  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  vtkSMProxy* lutProxy = vtkSMColorMapEditorHelper::GetBlockLUTProxy(proxy, view, blockSelector);
  if (!repr || !lutProxy)
  {
    return false;
  }

  return vtkSMTransferFunctionProxy::IsScalarBarVisible(lutProxy, view);
}

//----------------------------------------------------------------------------
vtkPVArrayInformation* vtkSMColorMapEditorHelper::GetArrayInformationForColorArray(
  vtkSMProxy* proxy, bool checkRepresentedData)
{
  if (!vtkSMColorMapEditorHelper::GetUsingScalarColoring(proxy))
  {
    return nullptr;
  }

  // now, determine a name for it if possible.
  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  const vtkSMPropertyHelper colorArrayHelper(proxy, "ColorArrayName");
  const vtkSMPropertyHelper inputHelper(proxy, "Input");
  vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  const unsigned int port = inputHelper.GetOutputPort();
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
vtkPVArrayInformation* vtkSMColorMapEditorHelper::GetBlockArrayInformationForColorArray(
  vtkSMProxy* proxy, const std::string& blockSelector, bool checkRepresentedData)
{
  if (!vtkSMColorMapEditorHelper::GetBlockUsingScalarColoring(proxy, blockSelector))
  {
    return nullptr;
  }

  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  // now, determine a name for it if possible.
  const auto attributeTypeAndArrayName =
    vtkSMColorMapEditorHelper::GetBlockColorArray(repr, blockSelector);
  const int attributeType = attributeTypeAndArrayName.first;
  const std::string arrayName = attributeTypeAndArrayName.second;

  const vtkSMPropertyHelper inputHelper(repr->GetProperty("Input"));
  vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  const int port = inputHelper.GetOutputPort();
  if (!inputProxy || !inputProxy->GetOutputPort(port))
  {
    vtkWarningWithObjectMacro(repr, "No input present. Cannot determine data ranges.");
    return nullptr;
  }

  if (inputProxy)
  {
    auto blockDataInfo =
      inputProxy->GetOutputPort(port)->GetSubsetDataInformation(blockSelector.c_str(),
        vtkSMPropertyHelper(repr->GetProperty("Assembly"), true).GetAsString());

    vtkPVArrayInformation* blockArrayInfo = nullptr;
    blockArrayInfo = blockDataInfo->GetArrayInformation(arrayName.c_str(), attributeType);
    if (blockArrayInfo)
    {
      return blockArrayInfo;
    }

    if (attributeType == vtkDataObject::POINT_THEN_CELL)
    {
      // Try points...
      blockArrayInfo = blockDataInfo->GetArrayInformation(arrayName.c_str(), vtkDataObject::POINT);
      if (blockArrayInfo)
      {
        return blockArrayInfo;
      }

      // ... then cells
      blockArrayInfo = blockDataInfo->GetArrayInformation(arrayName.c_str(), vtkDataObject::CELL);
      if (blockArrayInfo)
      {
        return blockArrayInfo;
      }
    }
  }

  if (checkRepresentedData)
  {
    vtkPVArrayInformation* arrayInfo =
      repr->GetRepresentedDataInformation()->GetArrayInformation(arrayName.c_str(), attributeType);
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
  if (!vtkSMColorMapEditorHelper::GetUsingScalarColoring(proxy))
  {
    return nullptr;
  }

  vtkPVArrayInformation* arrayInfo =
    vtkSMColorMapEditorHelper::GetArrayInformationForColorArray(proxy);
  if (!arrayInfo)
  {
    return nullptr;
  }

  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  const vtkSMPropertyHelper colorArrayHelper(proxy, "ColorArrayName");

  return repr->GetProminentValuesInformation(arrayInfo->GetName(),
    colorArrayHelper.GetInputArrayAssociation(), arrayInfo->GetNumberOfComponents(),
    uncertaintyAllowed, fraction, force);
}

//----------------------------------------------------------------------------
vtkPVProminentValuesInformation*
vtkSMColorMapEditorHelper::GetBlockProminentValuesInformationForColorArray(vtkSMProxy* proxy,
  const std::string& blockSelector, double uncertaintyAllowed, double fraction, bool force)
{
  if (!vtkSMColorMapEditorHelper::GetBlockUsingScalarColoring(proxy, blockSelector))
  {
    return nullptr;
  }

  vtkPVArrayInformation* blockArrayInfo =
    vtkSMColorMapEditorHelper::GetBlockArrayInformationForColorArray(proxy, blockSelector);
  if (!blockArrayInfo)
  {
    return nullptr;
  }

  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  const auto attributeTypeAndArrayName =
    vtkSMColorMapEditorHelper::GetBlockColorArray(repr, blockSelector);
  return repr->GetBlockProminentValuesInformation(blockSelector,
    vtkSMPropertyHelper(repr->GetProperty("Assembly"), true).GetAsString(),
    blockArrayInfo->GetName(), attributeTypeAndArrayName.first,
    blockArrayInfo->GetNumberOfComponents(), uncertaintyAllowed, fraction, force);
}

//----------------------------------------------------------------------------
int vtkSMColorMapEditorHelper::GetEstimatedNumberOfAnnotationsOnScalarBar(
  vtkSMProxy* proxy, vtkSMProxy* view)
{
  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  if (!repr || !view)
  {
    return -1;
  }

  if (!vtkSMColorMapEditorHelper::GetUsingScalarColoring(proxy))
  {
    return 0;
  }

  vtkSMProperty* lutProperty = proxy->GetProperty("LookupTable");
  if (!lutProperty)
  {
    vtkGenericWarningMacro("Missing 'LookupTable' property.");
    return -1;
  }

  const vtkSMPropertyHelper lutPropertyHelper(lutProperty);
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
int vtkSMColorMapEditorHelper::GetBlockEstimatedNumberOfAnnotationsOnScalarBar(
  vtkSMProxy* proxy, vtkSMProxy* view, const std::string& blockSelector)
{
  if (!vtkSMColorMapEditorHelper::GetBlockUsingScalarColoring(proxy, blockSelector))
  {
    return 0;
  }

  vtkSMProxy* blockLutProxy =
    vtkSMColorMapEditorHelper::GetBlockLUTProxy(proxy, view, blockSelector);
  if (!blockLutProxy)
  {
    return -1;
  }

  vtkNew<vtkSMTransferFunctionManager> mgr;
  vtkSMProxy* sbProxy = mgr->GetScalarBarRepresentation(blockLutProxy, view);
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
  auto repr = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
  return repr ? repr->GetLastLUTProxy() : nullptr;
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetLastLUTProxy(vtkSMProxy* proxy, vtkSMProxy* lutProxy)
{
  auto repr = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
  if (repr)
  {
    repr->SetLastLUTProxy(lutProxy);
  }
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMColorMapEditorHelper::GetLastBlockLUTProxy(
  vtkSMProxy* proxy, const std::string& blockSelector)
{
  auto repr = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
  return repr ? repr->GetLastBlockLUTProxy(blockSelector) : nullptr;
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetLastBlockLUTProxy(
  vtkSMProxy* proxy, const std::string& blockSelector, vtkSMProxy* lutProxy)
{
  auto repr = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
  if (repr)
  {
    repr->SetLastBlockLUTProxy(lutProxy, blockSelector);
  }
}
