// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMColorMapEditorHelper.h"

#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
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
#include "vtkVariant.h"

#include "vtksys/SystemTools.hxx"

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMColorMapEditorHelper);

//----------------------------------------------------------------------------
vtkSMColorMapEditorHelper::vtkSMColorMapEditorHelper() = default;

//----------------------------------------------------------------------------
vtkSMColorMapEditorHelper::~vtkSMColorMapEditorHelper() = default;

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent
     << "SelectedPropertiesType: " << (this->SelectedPropertiesType ? "Blocks" : "Representation")
     << "\n";
}

//----------------------------------------------------------------------------
vtkSMColorMapEditorHelper::SelectedPropertiesTypes vtkSMColorMapEditorHelper::GetPropertyType(
  vtkSMProperty* prop)
{
  if (!prop)
  {
    return SelectedPropertiesTypes::Representation;
  }
  return std::string(prop->GetXMLName()).substr(0, 5) == "Block"
    ? SelectedPropertiesTypes::Blocks
    : SelectedPropertiesTypes::Representation;
}

namespace
{
std::vector<std::string> GetItSelfAndParents(const std::string& blockSelector)
{
  std::vector<std::string> parts = vtksys::SystemTools::SplitString(blockSelector, '/', true);
  std::vector<std::string> paths;
  std::stringstream ss;
  for (size_t i = 1; i < parts.size(); ++i)
  {
    ss << "/" + parts[i];
    paths.push_back(ss.str());
  }
  // reverse to get the block selector first
  std::reverse(paths.begin(), paths.end());
  return paths;
}
}

//----------------------------------------------------------------------------
std::pair<std::string, vtkSMColorMapEditorHelper::BlockPropertyState>
vtkSMColorMapEditorHelper::HasBlockProperty(
  vtkSMProxy* proxy, const std::string& blockSelector, const std::string& propertyName)
{
  if (blockSelector.empty())
  {
    return { "/", BlockPropertyState::Disabled };
  }
  const auto blockProperty =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty(propertyName.c_str()));
  if (!blockProperty)
  {
    return { "/", BlockPropertyState::RepresentationInherited };
  }
  const std::vector<std::string> paths = ::GetItSelfAndParents(blockSelector);
  const int nEpC = blockProperty->GetNumberOfElementsPerCommand();
  for (size_t i = 0; i < paths.size(); ++i)
  {
    // the first path is the blockSelector itself
    const BlockPropertyState state =
      i == 0 ? BlockPropertyState::Set : BlockPropertyState::BlockInherited;
    for (unsigned int j = 0; j < blockProperty->GetNumberOfElements(); j += nEpC)
    {
      if (blockProperty->GetElement(j) == paths[i])
      {
        return { paths[i], state };
      }
    }
  }
  return { "/", BlockPropertyState::RepresentationInherited };
}

//----------------------------------------------------------------------------
std::pair<std::string, vtkSMColorMapEditorHelper::BlockPropertyState>
vtkSMColorMapEditorHelper::GetBlockPropertyStateFromBlockPropertyStates(
  const std::vector<std::pair<std::string, BlockPropertyState>>& blockPropertyStates)
{
  std::string path = "/";
  std::uint8_t state = BlockPropertyState::Disabled;
  for (const std::pair<std::string, BlockPropertyState>& blockPropertyState : blockPropertyStates)
  {
    if (blockPropertyState.second > state)
    {
      path = blockPropertyState.first;
    }
    state |= static_cast<std::uint8_t>(blockPropertyState.second);
  }
  return std::make_pair(path, static_cast<BlockPropertyState>(state));
}

//----------------------------------------------------------------------------
std::pair<std::string, vtkSMColorMapEditorHelper::BlockPropertyState>
vtkSMColorMapEditorHelper::HasBlockProperty(vtkSMProxy* proxy,
  const std::vector<std::string>& blockSelectors, const std::string& propertyName)
{
  if (blockSelectors.empty())
  {
    return { "/", BlockPropertyState::Disabled };
  }
  std::vector<std::pair<std::string, BlockPropertyState>> blockPropertyStates;
  for (const std::string& blockSelector : blockSelectors)
  {
    blockPropertyStates.push_back(
      vtkSMColorMapEditorHelper::HasBlockProperty(proxy, blockSelector, propertyName));
  }
  return vtkSMColorMapEditorHelper::GetBlockPropertyStateFromBlockPropertyStates(
    blockPropertyStates);
}

//----------------------------------------------------------------------------
std::pair<std::string, vtkSMColorMapEditorHelper::BlockPropertyState>
vtkSMColorMapEditorHelper::HasBlockProperties(vtkSMProxy* proxy, const std::string& blockSelector,
  const std::vector<std::string>& propertyNames)
{
  std::vector<std::pair<std::string, BlockPropertyState>> blockPropertyStates;
  for (const std::string& propertyName : propertyNames)
  {
    blockPropertyStates.push_back(
      vtkSMColorMapEditorHelper::HasBlockProperty(proxy, blockSelector, propertyName));
  }
  return vtkSMColorMapEditorHelper::GetBlockPropertyStateFromBlockPropertyStates(
    blockPropertyStates);
}

//----------------------------------------------------------------------------
std::pair<std::string, vtkSMColorMapEditorHelper::BlockPropertyState>
vtkSMColorMapEditorHelper::HasBlocksProperties(vtkSMProxy* proxy,
  const std::vector<std::string>& blockSelectors, const std::vector<std::string>& propertyNames)
{
  if (blockSelectors.empty())
  {
    return { "/", BlockPropertyState::Disabled };
  }
  std::vector<std::pair<std::string, BlockPropertyState>> blockPropertyStates;
  for (const std::string& blockSelector : blockSelectors)
  {
    blockPropertyStates.push_back(
      vtkSMColorMapEditorHelper::HasBlockProperties(proxy, blockSelector, propertyNames));
  }
  return vtkSMColorMapEditorHelper::GetBlockPropertyStateFromBlockPropertyStates(
    blockPropertyStates);
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(vtkSMProxy* proxy)
{
  const auto selectedBlockSelectorsProp =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("SelectedBlockSelectors"));
  if (selectedBlockSelectorsProp)
  {
    const std::vector<std::string>& elements = selectedBlockSelectorsProp->GetElements();
    if (elements.size() == 1)
    {
      return elements[0].empty() ? std::vector<std::string>() : elements;
    }
    else
    {
      return elements;
    }
  }
  else
  {
    return std::vector<std::string>();
  }
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkSMColorMapEditorHelper::GetColorArraysBlockSelectors(vtkSMProxy* proxy)
{
  const auto blockColorArray =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockColorArrayNames"));
  if (!blockColorArray)
  {
    vtkDebugWithObjectMacro(proxy, "No `BlockColorArrayNames` property found.");
    return std::vector<std::string>();
  }
  std::vector<std::string> blockSelectors;
  for (unsigned int i = 0; i < blockColorArray->GetNumberOfElements(); i += 3)
  {
    const std::string blockSelector = blockColorArray->GetElement(i);
    const std::string attributeTypeStr = blockColorArray->GetElement(i + 1);
    const std::string arrayName = blockColorArray->GetElement(i + 2);
    try
    {
      const int attributeType = std::stoi(attributeTypeStr);
      if (vtkSMColorMapEditorHelper::IsColorArrayValid(std::make_pair(attributeType, arrayName)))
      {
        blockSelectors.push_back(blockSelector);
      }
    }
    catch (const std::invalid_argument& e)
    {
      (void)e;
      vtkDebugWithObjectMacro(proxy, "Invalid attribute type: " << attributeTypeStr << e.what());
      break;
    }
  }
  return blockSelectors;
}

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
std::vector<vtkTypeBool> vtkSMColorMapEditorHelper::GetBlocksUsingScalarColoring(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return std::vector<vtkTypeBool>();
  }
  if (!vtkSMRepresentationProxy::SafeDownCast(proxy))
  {
    return std::vector<vtkTypeBool>(blockSelectors.size(), false);
  }
  const std::vector<ColorArray> blockColorArrays =
    vtkSMColorMapEditorHelper::GetBlocksColorArrays(proxy, blockSelectors);
  std::vector<vtkTypeBool> useScalarColorings(blockSelectors.size());
  for (size_t i = 0; i < blockSelectors.size(); ++i)
  {
    useScalarColorings[i] = vtkSMColorMapEditorHelper::IsColorArrayValid(blockColorArrays[i]);
  }
  return useScalarColorings;
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::GetAnyBlockUsingScalarColoring(vtkSMProxy* proxy)
{
  if (!vtkSMRepresentationProxy::SafeDownCast(proxy))
  {
    return false;
  }
  const std::vector<vtkTypeBool> blockUsingScalarColoring =
    vtkSMColorMapEditorHelper::GetBlocksUsingScalarColoring(
      proxy, vtkSMColorMapEditorHelper::GetColorArraysBlockSelectors(proxy));
  return std::any_of(blockUsingScalarColoring.begin(), blockUsingScalarColoring.end(),
    [](vtkTypeBool b) { return b; });
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMColorMapEditorHelper::GetSelectedUsingScalarColorings(
  vtkSMProxy* proxy)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    return vtkSMColorMapEditorHelper::GetBlocksUsingScalarColoring(
      proxy, vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy));
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    return { vtkSMColorMapEditorHelper::GetUsingScalarColoring(proxy) };
  }
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::GetAnySelectedUsingScalarColoring(vtkSMProxy* proxy)
{
  const std::vector<vtkTypeBool> useScalarColorings =
    vtkSMColorMapEditorHelper::GetSelectedUsingScalarColorings(proxy);
  return std::any_of(
    useScalarColorings.begin(), useScalarColorings.end(), [](vtkTypeBool b) { return b; });
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
        vtkSMProxy* lut = vtkSMTransferFunctionProxy::SafeDownCast(
          mgr->GetColorTransferFunction(arrayName, proxy->GetSessionProxyManager()));
        const int rescaleMode =
          vtkSMPropertyHelper(lut, "AutomaticRescaleRangeMode", true).GetAsInt();
        vtkSMPropertyHelper(lutProperty).Set(lut);
        const bool extend = rescaleMode == vtkSMTransferFunctionManager::GROW_ON_APPLY;
        const bool force = false;
        vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(proxy, extend, force);
        proxy->UpdateVTKObjects();
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetupBlocksLookupTables(vtkSMProxy* proxy)
{
  if (vtkSMColorMapEditorHelper::GetAnyBlockUsingScalarColoring(proxy))
  {
    // If representation has been initialized to use scalar coloring and no
    // transfer functions are setup, we setup the transfer functions.
    const auto repr = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    // get the selectors of the colored blocks
    const std::vector<std::string> blockSelectors =
      vtkSMColorMapEditorHelper::GetColorArraysBlockSelectors(proxy);
    const bool hasBlockLUT = repr->GetProperty("BlockLookupTables");
    // if the representation has a block lookup table property
    if (!hasBlockLUT)
    {
      return;
    }
    const std::map<ColorArray, std::vector<std::string>> commonColorArraysBlockSelectors =
      vtkSMColorMapEditorHelper::GetCommonColorArraysBlockSelectors(proxy, blockSelectors);
    vtkNew<vtkSMTransferFunctionManager> mgr;
    for (const auto& commonColorArrayBlockSelectors : commonColorArraysBlockSelectors)
    {
      if (!vtkSMColorMapEditorHelper::IsColorArrayValid(commonColorArrayBlockSelectors.first))
      {
        return;
      }
      const std::string arrayName = commonColorArrayBlockSelectors.first.second;
      const std::vector<std::string>& commonBlockSelectors = commonColorArrayBlockSelectors.second;
      vtkSMProxy* lut = vtkSMTransferFunctionProxy::SafeDownCast(
        mgr->GetColorTransferFunction(arrayName.c_str(), proxy->GetSessionProxyManager()));
      const int rescaleMode =
        vtkSMPropertyHelper(lut, "AutomaticRescaleRangeMode", true).GetAsInt();
      vtkSMColorMapEditorHelper::SetBlocksLookupTable(proxy, commonBlockSelectors, lut);
      const bool extend = rescaleMode == vtkSMTransferFunctionManager::GROW_ON_APPLY;
      const bool force = false;
      vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRange(
        proxy, commonBlockSelectors, extend, force);
      proxy->UpdateVTKObjects();
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
std::vector<vtkTypeBool> vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRange(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors, bool extend, bool force)
{
  if (blockSelectors.empty())
  {
    return std::vector<vtkTypeBool>();
  }
  SM_SCOPED_TRACE(CallMethod)
    .arg(proxy)
    .arg("RescaleBlocksTransferFunctionToDataRange")
    .arg(blockSelectors)
    .arg(extend)
    .arg(force)
    .arg("comment",
      (extend ? "rescale block(s) color and/or opacity maps used to include current data range"
              : "rescale block(s) color and/or opacity maps used to exactly fit the current data "
                "range"));
  return vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRange(proxy, blockSelectors,
    vtkSMColorMapEditorHelper::GetBlocksArrayInformationForColorArray(proxy, blockSelectors),
    extend, force);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRange(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors, const char* arrayName,
  int attributeType, bool extend, bool force)
{
  if (blockSelectors.empty())
  {
    return std::vector<vtkTypeBool>();
  }
  const auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  const vtkSMPropertyHelper inputHelper(proxy->GetProperty("Input"));
  vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  const int port = inputHelper.GetOutputPort();
  if (!inputProxy || !inputProxy->GetOutputPort(port))
  {
    // no input.
    vtkDebugWithObjectMacro(proxy, "No input present. Cannot determine data ranges.");
    return std::vector<vtkTypeBool>(blockSelectors.size(), false);
  }

  std::vector<vtkPVArrayInformation*> arrayInfos(blockSelectors.size(), nullptr);
  for (size_t i = 0; i < blockSelectors.size(); ++i)
  {
    vtkPVDataInformation* blockDataInfo = inputProxy->GetOutputPort(port)->GetSubsetDataInformation(
      blockSelectors[i].c_str(), vtkSMPropertyHelper(proxy, "Assembly", true).GetAsString());
    vtkPVArrayInformation* blockArrayInfo =
      blockDataInfo->GetArrayInformation(arrayName, attributeType);
    if (!blockArrayInfo)
    {
      vtkPVDataInformation* representedDataInfo = repr->GetRepresentedDataInformation();
      blockArrayInfo = representedDataInfo->GetArrayInformation(arrayName, attributeType);
    }
    arrayInfos[i] = blockArrayInfo;
  }
  return vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRange(
    repr, blockSelectors, arrayInfos, extend, force);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMColorMapEditorHelper::RescaleSelectedTransferFunctionToDataRange(
  vtkSMProxy* proxy, bool extend, bool force)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    return vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRange(
      proxy, vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy), extend, force);
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    return { vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(proxy, extend, force) };
  }
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMColorMapEditorHelper::RescaleSelectedTransferFunctionToDataRange(
  vtkSMProxy* proxy, const char* arrayName, int attributeType, bool extend, bool force)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    return vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRange(proxy,
      vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy), arrayName, attributeType, extend,
      force);
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    return { vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(
      proxy, arrayName, attributeType, extend, force) };
  }
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
  return info ? vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(proxy, info) : false;
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool>
vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRangeOverTime(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return std::vector<vtkTypeBool>();
  }
  const std::map<ColorArray, std::vector<std::string>> commonColorArraysBlockSelectors =
    vtkSMColorMapEditorHelper::GetCommonColorArraysBlockSelectors(proxy, blockSelectors);

  std::map<ColorArray, std::vector<vtkTypeBool>> arrayResults;
  for (const auto& commonColorArrayBlockSelectors : commonColorArraysBlockSelectors)
  {
    if (!vtkSMColorMapEditorHelper::IsColorArrayValid(commonColorArrayBlockSelectors.first))
    {
      return std::vector<vtkTypeBool>(commonColorArrayBlockSelectors.second.size(), false);
    }
    const std::string arrayName = commonColorArrayBlockSelectors.first.second;
    const std::vector<std::string>& commonBlockSelectors = commonColorArrayBlockSelectors.second;
    arrayResults[commonColorArrayBlockSelectors.first] =
      vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRangeOverTime(
        proxy, commonBlockSelectors, arrayName.c_str(), commonColorArrayBlockSelectors.first.first);
  }
  if (arrayResults.size() == 1)
  {
    return arrayResults.begin()->second;
  }
  else
  {
    const std::vector<ColorArray> blocksColorArrays =
      vtkSMColorMapEditorHelper::GetBlocksColorArrays(proxy, blockSelectors);
    std::vector<vtkTypeBool> results(blockSelectors.size(), false);
    for (size_t i = 0; i < blockSelectors.size(); ++i)
    {
      const ColorArray& blockColorArray = blocksColorArrays[i];
      const std::string selector = blockSelectors[i];
      const std::vector<int>& colorArrayResults = arrayResults.find(blockColorArray)->second;
      const std::vector<std::string>& colorArrayBlockSelectors =
        commonColorArraysBlockSelectors.find(blockColorArray)->second;
      for (size_t j = 0; j < colorArrayResults.size(); ++j)
      {
        if (colorArrayBlockSelectors[j] == selector)
        {
          results[i] = colorArrayResults[j];
          break;
        }
      }
    }
    return results;
  }
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool>
vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRangeOverTime(vtkSMProxy* proxy,
  const std::vector<std::string>& blockSelectors, const char* arrayName, int attributeType)
{
  if (blockSelectors.empty())
  {
    return std::vector<vtkTypeBool>();
  }
  const vtkSMPropertyHelper inputHelper(proxy->GetProperty("Input"));
  const auto inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  const int port = inputHelper.GetOutputPort();
  if (!inputProxy || !inputProxy->GetOutputPort(port))
  {
    // no input.
    vtkDebugWithObjectMacro(proxy, "No input present. Cannot determine data ranges.");
    return std::vector<vtkTypeBool>(blockSelectors.size(), false);
  }

  std::vector<vtkPVArrayInformation*> arrayInfos(blockSelectors.size(), nullptr);
  for (size_t i = 0; i < blockSelectors.size(); ++i)
  {
    vtkPVTemporalDataInformation* blockDataInfo =
      inputProxy->GetOutputPort(port)->GetTemporalSubsetDataInformation(
        blockSelectors[i].c_str(), vtkSMPropertyHelper(proxy, "Assembly", true).GetAsString());
    vtkPVArrayInformation* blockArrayInfo =
      blockDataInfo->GetArrayInformation(arrayName, attributeType);
    arrayInfos[i] = blockArrayInfo;
  }
  return vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRange(
    proxy, blockSelectors, arrayInfos);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool>
vtkSMColorMapEditorHelper::RescaleSelectedTransferFunctionToDataRangeOverTime(vtkSMProxy* proxy)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    return vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRangeOverTime(
      proxy, vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy));
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    return { vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRangeOverTime(proxy) };
  }
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool>
vtkSMColorMapEditorHelper::RescaleSelectedTransferFunctionToDataRangeOverTime(
  vtkSMProxy* proxy, const char* arrayName, int attributeType)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    return vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRangeOverTime(
      proxy, vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy), arrayName, attributeType);
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    return { vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRangeOverTime(
      proxy, arrayName, attributeType) };
  }
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
      vtkSmartPointer<vtkAbstractArray> uniqueValues = vtk::TakeSmartPointer(
        vtkSMColorMapEditorHelper::GetProminentValuesInformationForColorArray(proxy)
          ->GetProminentComponentValues(component));

      vtkSMStringVectorProperty* allAnnotations =
        vtkSMStringVectorProperty::SafeDownCast(lut->GetProperty("Annotations"));
      vtkSMStringVectorProperty* activeAnnotatedValuesProperty =
        vtkSMStringVectorProperty::SafeDownCast(lut->GetProperty("ActiveAnnotatedValues"));
      if (uniqueValues && allAnnotations && activeAnnotatedValuesProperty)
      {
        vtkNew<vtkStringList> activeAnnotatedValues;
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
      if (vtkSMProperty* uoaProperty = proxy->GetProperty("UseSeparateOpacityArray"))
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
std::vector<vtkTypeBool> vtkSMColorMapEditorHelper::RescaleBlocksTransferFunctionToDataRange(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors,
  std::vector<vtkPVArrayInformation*> infos, bool extend, bool force)
{
  if (blockSelectors.empty())
  {
    return std::vector<vtkTypeBool>();
  }
  const std::vector<vtkTypeBool> blockUsingScalarColoring =
    vtkSMColorMapEditorHelper::GetBlocksUsingScalarColoring(proxy, blockSelectors);
  // if no block is using scalar coloring
  if (!std::any_of(blockUsingScalarColoring.begin(), blockUsingScalarColoring.end(),
        [](vtkTypeBool b) { return b; }))
  {
    // nothing to do.
    return std::vector<vtkTypeBool>(blockSelectors.size(), false);
  }

  std::vector<vtkSMProxy*> blockLuts =
    vtkSMColorMapEditorHelper::GetBlocksLookupTables(proxy, blockSelectors);

  std::vector<vtkPVProminentValuesInformation*> blockProminentValues =
    vtkSMColorMapEditorHelper::GetBlocksProminentValuesInformationForColorArray(
      proxy, blockSelectors);

  std::vector<vtkTypeBool> results(blockSelectors.size(), false);
  for (size_t i = 0; i < blockSelectors.size(); ++i)
  {
    vtkPVArrayInformation* info = infos[i];
    if (!info)
    {
      results[i] = false;
      continue;
    }

    vtkSMProxy* blockLut = blockLuts[i];
    if (!blockLut)
    {
      results[i] = false;
      continue;
    }

    if (force == false &&
      vtkSMPropertyHelper(blockLut, "AutomaticRescaleRangeMode", true).GetAsInt() ==
        vtkSMTransferFunctionManager::NEVER)
    {
      // nothing to change, range is locked.
      results[i] = true;
      continue;
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
      if (indexedLookup > 0)
      {
        vtkSmartPointer<vtkAbstractArray> uniqueValues =
          vtk::TakeSmartPointer(blockProminentValues[i]->GetProminentComponentValues(component));

        const auto allAnnotations =
          vtkSMStringVectorProperty::SafeDownCast(blockLut->GetProperty("Annotations"));
        const auto activeAnnotatedValuesProperty =
          vtkSMStringVectorProperty::SafeDownCast(blockLut->GetProperty("ActiveAnnotatedValues"));
        if (uniqueValues && allAnnotations && activeAnnotatedValuesProperty)
        {
          vtkNew<vtkStringList> activeAnnotatedValues;
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
        results[i] = (blockLut != nullptr);
      }
    }
    else
    {
      results[i] = false;
    }
  }
  return results;
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
  const auto inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
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
  vtkPVArrayInformation* arrayInfo =
    vtkSMColorMapEditorHelper::GetArrayInformationForColorArray(proxy, false);
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
  vtkSMProxy* lut = nullptr;
  if (vtkSMProperty* lutProperty = proxy->GetProperty("LookupTable"))
  {
    lut =
      mgr->GetColorTransferFunction(decoratedArrayName.c_str(), proxy->GetSessionProxyManager());
    if (useComponent || forceComponentMode)
    {
      if (component >= 0)
      {
        vtkSMPropertyHelper(lut, "VectorMode").Set("Component");
        vtkSMPropertyHelper(lut, "VectorComponent").Set(component);
        lut->UpdateVTKObjects();
      }
      else
      {
        vtkSMPropertyHelper(lut, "VectorMode").Set("Magnitude");
        lut->UpdateVTKObjects();
      }
    }
    else
    {
      // No Component defined for coloring, in order to generate a valid trace
      // a component is needed, recover currently used component
      const char* vectorMode = vtkSMPropertyHelper(lut, "VectorMode").GetAsString();
      haveComponent = true;
      if (strcmp(vectorMode, "Component") == 0)
      {
        component = vtkSMPropertyHelper(lut, "VectorComponent").GetAsInt();
      }
      else // Magnitude
      {
        component = -1;
      }
    }

    vtkSMPropertyHelper(lutProperty).Set(lut);

    // Get the array information for the color array to determine transfer function properties
    vtkPVArrayInformation* colorArrayInfo =
      vtkSMColorMapEditorHelper::GetArrayInformationForColorArray(proxy);
    if (colorArrayInfo)
    {
      if (colorArrayInfo->GetDataType() == VTK_STRING)
      {
        vtkSMPropertyHelper(lut, "IndexedLookup", true).Set(1);
        lut->UpdateVTKObjects();
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
    if (lut && useTransfer2D)
    {
      vtkSMPropertyHelper(lut, "Using2DTransferFunction").Set(useTransfer2D);
      lut->UpdateVTKObjects();
    }
  }

  proxy->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMColorMapEditorHelper::SetBlocksScalarColoring(vtkSMProxy* proxy,
  const std::vector<std::string>& blockSelectors, const char* arrayName, int attributeType)
{
  if (blockSelectors.empty())
  {
    return std::vector<vtkTypeBool>();
  }
  return vtkSMColorMapEditorHelper::SetBlocksScalarColoringInternal(
    proxy, blockSelectors, arrayName, attributeType, false, -1);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMColorMapEditorHelper::SetBlocksScalarColoring(vtkSMProxy* proxy,
  const std::vector<std::string>& blockSelectors, const char* arrayName, int attributeType,
  int component)
{
  if (blockSelectors.empty())
  {
    return std::vector<vtkTypeBool>();
  }
  return vtkSMColorMapEditorHelper::SetBlocksScalarColoringInternal(
    proxy, blockSelectors, arrayName, attributeType, true, component);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMColorMapEditorHelper::SetSelectedScalarColoring(
  vtkSMProxy* proxy, const char* arrayName, int attributeType)
{
  return vtkSMColorMapEditorHelper::SetSelectedScalarColoringInternal(
    proxy, arrayName, attributeType, false, -1);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMColorMapEditorHelper::SetSelectedScalarColoring(
  vtkSMProxy* proxy, const char* arrayName, int attributeType, int component)
{
  return vtkSMColorMapEditorHelper::SetSelectedScalarColoringInternal(
    proxy, arrayName, attributeType, true, component);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMColorMapEditorHelper::SetBlocksScalarColoringInternal(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors, const char* arrayName,
  int attributeType, bool useComponent, int component)
{
  if (blockSelectors.empty())
  {
    return std::vector<vtkTypeBool>();
  }
  const auto repr = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
  if (!repr)
  {
    return std::vector<vtkTypeBool>(blockSelectors.size(), false);
  }
  const std::vector<vtkTypeBool> blockUsingScalarColorings =
    vtkSMColorMapEditorHelper::GetBlocksUsingScalarColoring(proxy, blockSelectors);

  // if all are not using scalar coloring
  if (std::all_of(blockUsingScalarColorings.begin(), blockUsingScalarColorings.end(),
        [](vtkTypeBool b) { return !b; }) &&
    (arrayName == nullptr || arrayName[0] == 0))
  {
    // scalar coloring already off. Nothing to do.
    return std::vector<vtkTypeBool>(blockSelectors.size(), true);
  }

  // if any block is using scalar coloring and no array name provided
  if (arrayName == nullptr || arrayName[0] == 0)
  {
    SM_SCOPED_TRACE(SetBlocksScalarColoring)
      .arg("display", repr)
      .arg("block_selectors", blockSelectors)
      .arg("arrayname", arrayName)
      .arg("attribute_type", attributeType);

    // IMPORTANT!!! First remove luts and then color arrays
    vtkSMColorMapEditorHelper::RemoveBlocksLookupTables(repr, blockSelectors);
    vtkSMColorMapEditorHelper::RemoveBlocksColorArrays(repr, blockSelectors);
    // Scalar Opacity is not supported per block yet
    repr->UpdateVTKObjects();
    return std::vector<vtkTypeBool>(blockSelectors.size(), true);
  }
  else
  {
    vtkSMColorMapEditorHelper::SetBlocksColorArray(repr, blockSelectors, attributeType, arrayName);
  }

  auto* arraySettings = vtkPVRepresentedArrayListSettings::GetInstance();
  const std::vector<vtkPVArrayInformation*> blockArrayInfos =
    vtkSMColorMapEditorHelper::GetBlocksArrayInformationForColorArray(repr, blockSelectors);
  // if there is no block's array information that is available
  if (std::all_of(blockArrayInfos.begin(), blockArrayInfos.end(),
        [](vtkPVArrayInformation* info) { return info == nullptr; }))
  {
    vtkWarningWithObjectMacro(repr, "Could not determine array information.");
    return std::vector<vtkTypeBool>(blockSelectors.size(), false);
  }
  // get first valid block's array information
  int firstValidInfoIndex = -1;
  for (size_t i = 0; i < blockArrayInfos.size(); ++i)
  {
    if (blockArrayInfos[i])
    {
      firstValidInfoIndex = static_cast<int>(i);
      break;
    }
  }

  // since all blocks should use the same array, infos should have the same number of components
  const bool forceComponentMode = (blockArrayInfos[firstValidInfoIndex] && arraySettings &&
    !arraySettings->ShouldUseMagnitudeMode(
      blockArrayInfos[firstValidInfoIndex]->GetNumberOfComponents()));
  if (forceComponentMode && (!useComponent || component < 0))
  {
    component = 0;
  }

  const bool hasBlockLUTProperty = repr->GetProperty("BlockLookupTables") != nullptr;
  if (!hasBlockLUTProperty)
  {
    vtkDebugWithObjectMacro(repr, "No 'BlockLookupTables' found.");
    return std::vector<vtkTypeBool>(blockSelectors.size(), false);
  }

  const std::vector<int> separates =
    vtkSMColorMapEditorHelper::GetBlocksUseSeparateColorMaps(repr, blockSelectors);
  // if all blocks are not using separate color maps
  const bool sameLut =
    std::all_of(separates.begin(), separates.end(), [](int s) { return s != 1; });

  const std::vector<std::string> decoratedArrayNames =
    vtkSMColorMapEditorHelper::GetBlocksDecoratedArrayNames(repr, blockSelectors, arrayName);

  vtkNew<vtkSMTransferFunctionManager> mgr;
  bool haveComponent = useComponent || forceComponentMode ? useComponent : true;
  for (size_t i = 0; i < blockSelectors.size(); ++i)
  {
    vtkSMProxy* blockLut =
      mgr->GetColorTransferFunction(decoratedArrayNames[i].c_str(), repr->GetSessionProxyManager());
    if (useComponent || forceComponentMode)
    {
      if (component >= 0)
      {
        vtkSMPropertyHelper(blockLut, "VectorMode").Set("Component");
        vtkSMPropertyHelper(blockLut, "VectorComponent").Set(component);
        blockLut->UpdateVTKObjects();
      }
      else
      {
        vtkSMPropertyHelper(blockLut, "VectorMode").Set("Magnitude");
        blockLut->UpdateVTKObjects();
      }
    }
    else
    {
      // No Component defined for coloring, in order to generate a valid trace
      // a component is needed, recover currently used component
      const char* vectorMode = vtkSMPropertyHelper(blockLut, "VectorMode").GetAsString();
      if (strcmp(vectorMode, "Component") == 0)
      {
        // Many blocks can override the component because it should be the same for all.
        component = vtkSMPropertyHelper(blockLut, "VectorComponent").GetAsInt();
      }
      else // Magnitude
      {
        // Many blocks can override the component because it should be the same for all.
        component = -1;
      }
    }
    // Stop quickly if all blocks will be using the same lut
    if (sameLut)
    {
      vtkSMColorMapEditorHelper::SetBlocksLookupTable(repr, blockSelectors, blockLut);
      break;
    }
    else
    {
      vtkSMColorMapEditorHelper::SetBlockLookupTable(repr, blockSelectors[i], blockLut);
    }
  }

  // get component name
  std::string componentName;
  if (haveComponent)
  {
    // the component name is the same for all blocks
    componentName = blockArrayInfos[firstValidInfoIndex]->GetComponentName(component);
    haveComponent = !componentName.empty();
  }

  for (size_t i = 0; i < blockSelectors.size(); ++i)
  {
    vtkSMProxy* blockLut =
      mgr->GetColorTransferFunction(decoratedArrayNames[i].c_str(), repr->GetSessionProxyManager());
    if (blockLut && blockArrayInfos[i] && blockArrayInfos[i]->GetDataType() == VTK_STRING)
    {
      vtkSMPropertyHelper(blockLut, "IndexedLookup", true).Set(1);
      blockLut->UpdateVTKObjects();
    }
  }

  if (haveComponent)
  {
    SM_SCOPED_TRACE(SetBlocksScalarColoring)
      .arg("display", repr)
      .arg("block_selectors", blockSelectors)
      .arg("arrayname", arrayName)
      .arg("attribute_type", attributeType)
      .arg("component", componentName.c_str())
      .arg("separate", !sameLut);
  }
  else
  {
    SM_SCOPED_TRACE(SetBlocksScalarColoring)
      .arg("display", repr)
      .arg("block_selectors", blockSelectors)
      .arg("arrayname", arrayName)
      .arg("attribute_type", attributeType)
      .arg("separate", !sameLut);
  }
  repr->UpdateVTKObjects();

  return std::vector<vtkTypeBool>(blockSelectors.size(), true);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMColorMapEditorHelper::SetSelectedScalarColoringInternal(
  vtkSMProxy* proxy, const char* arrayName, int attributeType, bool useComponent, int component)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    return vtkSMColorMapEditorHelper::SetBlocksScalarColoringInternal(proxy,
      vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy), arrayName, attributeType,
      useComponent, component);
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    return { vtkSMColorMapEditorHelper::SetScalarColoringInternal(
      proxy, arrayName, attributeType, useComponent, component) };
  }
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
std::vector<std::string> vtkSMColorMapEditorHelper::GetBlocksDecoratedArrayNames(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors, const std::string& arrayName)
{
  if (blockSelectors.empty())
  {
    return std::vector<std::string>();
  }
  std::vector<int> blocksUseSeparateColorMap =
    vtkSMColorMapEditorHelper::GetBlocksUseSeparateColorMaps(proxy, blockSelectors);
  const int representationSeparateColorMap =
    vtkSMPropertyHelper(proxy, "UseSeparateColorMap", true).GetAsInt();

  std::ostringstream ss;
  ss << arrayName;
  std::vector<std::string> decoratedArrayNames(blockSelectors.size());
  for (size_t i = 0; i < blockSelectors.size(); ++i)
  {
    if (blocksUseSeparateColorMap[i] == 1)
    {
      std::ostringstream ss1;
      ss1 << "Separate_" << proxy->GetGlobalIDAsString() << "_" << blockSelectors[i] << "_"
          << ss.str();
      decoratedArrayNames[i] = ss1.str();
    }
    else if (representationSeparateColorMap == 1)
    {
      std::ostringstream ss1;
      ss1 << "Separate_" << proxy->GetGlobalIDAsString() << "_" << blockSelectors[i] << "_"
          << ss.str();
      decoratedArrayNames[i] = ss1.str();
    }
    else
    {
      decoratedArrayNames[i] = ss.str();
    }
  }
  return decoratedArrayNames;
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::IsColorValid(Color color)
{
  return std::all_of(color.begin(), color.end(), [](double c) { return c >= 0.0 && c <= 1.0; });
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetColor(vtkSMProxy* proxy, Color color)
{
  if (!vtkSMColorMapEditorHelper::IsColorValid(color))
  {
    vtkWarningWithObjectMacro(proxy, "Invalid color.");
    return;
  }
  vtkSMProperty* diffuse = proxy->GetProperty("DiffuseColor");
  vtkSMProperty* ambient = proxy->GetProperty("AmbientColor");
  if (diffuse == nullptr && ambient == nullptr)
  {
    diffuse = proxy->GetProperty("Color");
  }
  SM_SCOPED_TRACE(PropertiesModified).arg("proxy", proxy).arg("comment", " change solid color");
  vtkSMPropertyHelper(diffuse, /*quiet=*/true).Set(color.data(), 3);
  vtkSMPropertyHelper(ambient, /*quiet=*/true).Set(color.data(), 3);
  proxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetBlocksColor(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors, Color color)
{
  if (blockSelectors.empty())
  {
    return;
  }
  if (!vtkSMColorMapEditorHelper::IsColorValid(color))
  {
    vtkWarningWithObjectMacro(proxy, "Invalid color.");
    return;
  }
  auto blockColorsProp = vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockColors"));
  if (!blockColorsProp)
  {
    vtkDebugWithObjectMacro(proxy, "No 'BlockColors' property found.");
    return;
  }
  assert(blockColorsProp->GetNumberOfElementsPerCommand() == 4);
  std::map<std::string, Color> blockColorsMap;
  for (unsigned int i = 0; i < blockColorsProp->GetNumberOfElements(); i += 4)
  {
    blockColorsMap[blockColorsProp->GetElement(i)] = {
      std::stod(blockColorsProp->GetElement(i + 1)), std::stod(blockColorsProp->GetElement(i + 2)),
      std::stod(blockColorsProp->GetElement(i + 3))
    };
  }
  for (const std::string& blockSelector : blockSelectors)
  {
    blockColorsMap[blockSelector] = color;
  }
  std::vector<std::string> blockColorsVec;
  blockColorsVec.reserve(blockColorsMap.size() * 4);
  for (const auto& blockColor : blockColorsMap)
  {
    blockColorsVec.push_back(blockColor.first);
    blockColorsVec.push_back(std::to_string(blockColor.second[0]));
    blockColorsVec.push_back(std::to_string(blockColor.second[1]));
    blockColorsVec.push_back(std::to_string(blockColor.second[2]));
  }
  SM_SCOPED_TRACE(PropertiesModified)
    .arg("proxy", proxy)
    .arg("comment", " change block(s) solid Color");
  blockColorsProp->SetElements(blockColorsVec);
  proxy->UpdateProperty("BlockColors");
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::RemoveBlocksColors(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return;
  }
  auto blockColorsProp = vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockColors"));
  if (!blockColorsProp)
  {
    vtkDebugWithObjectMacro(proxy, "No 'BlockColors' property found.");
    return;
  }
  assert(blockColorsProp->GetNumberOfElementsPerCommand() == 4);
  std::map<std::string, Color> blockColorsMap;
  for (unsigned int i = 0; i < blockColorsProp->GetNumberOfElements(); i += 4)
  {
    blockColorsMap[blockColorsProp->GetElement(i)] = {
      std::stod(blockColorsProp->GetElement(i + 1)), std::stod(blockColorsProp->GetElement(i + 2)),
      std::stod(blockColorsProp->GetElement(i + 3))
    };
  }
  for (const std::string& blockSelector : blockSelectors)
  {
    blockColorsMap.erase(blockSelector);
  }
  std::vector<std::string> blockColorsVec;
  blockColorsVec.reserve(blockColorsMap.size() * 4);
  for (const auto& blockColor : blockColorsMap)
  {
    blockColorsVec.push_back(blockColor.first);
    blockColorsVec.push_back(std::to_string(blockColor.second[0]));
    blockColorsVec.push_back(std::to_string(blockColor.second[1]));
    blockColorsVec.push_back(std::to_string(blockColor.second[2]));
  }
  SM_SCOPED_TRACE(PropertiesModified)
    .arg("proxy", proxy)
    .arg("comment", " change block(s) solid Color");
  blockColorsProp->SetElements(blockColorsVec);
  proxy->UpdateProperty("BlockColors");
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetSelectedColor(vtkSMProxy* proxy, Color color)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    vtkSMColorMapEditorHelper::SetBlocksColor(
      proxy, vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy), color);
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    vtkSMColorMapEditorHelper::SetColor(proxy, color);
  }
}

//----------------------------------------------------------------------------
vtkSMColorMapEditorHelper::Color vtkSMColorMapEditorHelper::GetColor(vtkSMProxy* proxy)
{
  vtkSMProperty* diffuse = proxy->GetProperty("DiffuseColor");
  vtkSMProperty* ambient = proxy->GetProperty("AmbientColor");
  if (diffuse == nullptr && ambient == nullptr)
  {
    diffuse = proxy->GetProperty("Color");
  }
  Color color;
  if (diffuse || ambient)
  {
    vtkSMPropertyHelper(diffuse ? diffuse : ambient).Get(color.data(), 3);
  }
  else
  {
    color[0] = color[1] = color[2] = VTK_DOUBLE_MAX;
  }
  return color;
}

//----------------------------------------------------------------------------
std::vector<vtkSMColorMapEditorHelper::Color> vtkSMColorMapEditorHelper::GetBlocksColors(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return std::vector<Color>();
  }
  const auto blockColorsProp =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockColors"));
  if (!blockColorsProp)
  {
    vtkDebugWithObjectMacro(proxy, "No 'BlockColors' property found.");
    return std::vector<Color>{ blockSelectors.size(),
      { VTK_DOUBLE_MAX, VTK_DOUBLE_MAX, VTK_DOUBLE_MAX } };
  }
  std::vector<Color> colors(
    blockSelectors.size(), { VTK_DOUBLE_MAX, VTK_DOUBLE_MAX, VTK_DOUBLE_MAX });
  for (unsigned int i = 0; i < blockColorsProp->GetNumberOfElements(); i += 4)
  {
    for (unsigned int j = 0; j < blockSelectors.size(); ++j)
    {
      if (blockColorsProp->GetElement(i) == blockSelectors[j])
      {
        colors[j] = { std::stod(blockColorsProp->GetElement(i + 1)),
          std::stod(blockColorsProp->GetElement(i + 2)),
          std::stod(blockColorsProp->GetElement(i + 3)) };
        break;
      }
    }
  }
  return colors;
}

//----------------------------------------------------------------------------
std::vector<vtkSMColorMapEditorHelper::Color> vtkSMColorMapEditorHelper::GetSelectedColors(
  vtkSMProxy* proxy)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    return vtkSMColorMapEditorHelper::GetBlocksColors(
      proxy, vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy));
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    return { vtkSMColorMapEditorHelper::GetColor(proxy) };
  }
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkSMColorMapEditorHelper::GetColorArrayProperty(vtkSMProxy* proxy)
{
  return proxy->GetProperty("ColorArrayName");
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkSMColorMapEditorHelper::GetBlockColorArrayProperty(vtkSMProxy* proxy)
{
  return proxy->GetProperty("BlockColorArrayNames");
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkSMColorMapEditorHelper::GetSelectedColorArrayProperty(vtkSMProxy* proxy)
{
  return this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks
    ? vtkSMColorMapEditorHelper::GetBlockColorArrayProperty(proxy)
    : vtkSMColorMapEditorHelper::GetColorArrayProperty(proxy);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::IsColorArrayValid(const ColorArray& array)
{
  return array.first >= vtkDataObject::POINT &&
    array.first < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES && !array.second.empty();
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetColorArray(
  vtkSMProxy* proxy, int attributeType, std::string arrayName)
{
  if (!vtkSMColorMapEditorHelper::IsColorArrayValid(std::make_pair(attributeType, arrayName)))
  {
    vtkWarningWithObjectMacro(proxy, "Invalid color array.");
    return;
  }
  SM_SCOPED_TRACE(PropertiesModified).arg("proxy", proxy).arg("comment", " change color array");
  vtkSMPropertyHelper(proxy, "ColorArrayName")
    .SetInputArrayToProcess(attributeType, arrayName.c_str());
  proxy->UpdateProperty("ColorArrayName");
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetBlocksColorArray(vtkSMProxy* proxy,
  const std::vector<std::string>& blockSelectors, int attributeType, std::string arrayName)
{
  if (blockSelectors.empty())
  {
    return;
  }
  if (!vtkSMColorMapEditorHelper::IsColorArrayValid(std::make_pair(attributeType, arrayName)))
  {
    vtkWarningWithObjectMacro(proxy, "Invalid color array.");
    return;
  }
  auto blockColorArrayNames =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockColorArrayNames"));
  auto blockLUTs = vtkSMProxyProperty::SafeDownCast(proxy->GetProperty("BlockLookupTables"));
  if (!blockColorArrayNames || !blockLUTs)
  {
    vtkDebugWithObjectMacro(
      proxy, "No 'BlockColorArrayNames' or 'BlockLookupTables' properties found.");
    return;
  }
  assert(blockColorArrayNames->GetNumberOfElementsPerCommand() == 3);
  if (arrayName.empty())
  {
    return;
  }
  std::map<std::string, ColorArray> blockColorArrayMap;
  std::map<std::string, vtkSMProxy*> blockLUTsMap;
  for (unsigned int i = 0; i < blockLUTs->GetNumberOfProxies(); ++i)
  {
    blockColorArrayMap[blockColorArrayNames->GetElement(3 * i)] =
      std::make_pair(std::stoi(blockColorArrayNames->GetElement(3 * i + 1)),
        std::string(blockColorArrayNames->GetElement(3 * i + 2)));
    blockLUTsMap[blockColorArrayNames->GetElement(3 * i)] = blockLUTs->GetProxy(i);
  }
  for (const std::string& blockSelector : blockSelectors)
  {
    blockColorArrayMap[blockSelector] = std::make_pair(attributeType, arrayName);
    blockLUTsMap[blockSelector] = nullptr;
  }
  std::vector<std::string> blockColorArrayVec;
  blockColorArrayVec.reserve(blockColorArrayMap.size() * 3);
  std::vector<vtkSMProxy*> blockLUTsVec;
  blockLUTsVec.reserve(blockLUTsMap.size());
  for (const auto& colorArray : blockColorArrayMap)
  {
    blockColorArrayVec.push_back(colorArray.first);
    blockColorArrayVec.push_back(std::to_string(colorArray.second.first));
    blockColorArrayVec.push_back(colorArray.second.second);
    blockLUTsVec.push_back(blockLUTsMap[colorArray.first]);
  }
  // the following trace is not needed because this function is not supposed to be publicly called
  // SM_SCOPED_TRACE(PropertiesModified)
  //   .arg("proxy", proxy)
  //   .arg("comment", " change block(s) color array");
  blockColorArrayNames->SetElements(blockColorArrayVec);
  proxy->UpdateProperty("BlockColorArrayNames");
  blockLUTs->SetProxies(static_cast<unsigned int>(blockLUTsVec.size()), blockLUTsVec.data());
  // no need to call UpdateProperty for BlockLookupTables since SetBlocksLookupTable will do that
}
//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::RemoveBlocksColorArrays(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return;
  }
  auto blockColorArrayNames =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockColorArrayNames"));
  if (!blockColorArrayNames)
  {
    vtkDebugWithObjectMacro(proxy, "No 'BlockColorArrayNames' property found.");
    return;
  }
  assert(blockColorArrayNames->GetNumberOfElementsPerCommand() == 3);
  std::map<std::string, ColorArray> blockColorArrayMap;
  for (unsigned int i = 0; i < blockColorArrayNames->GetNumberOfElements(); i += 3)
  {
    blockColorArrayMap[blockColorArrayNames->GetElement(i)] =
      std::make_pair(std::stoi(blockColorArrayNames->GetElement(i + 1)),
        std::string(blockColorArrayNames->GetElement(i + 2)));
  }
  for (const std::string& blockSelector : blockSelectors)
  {
    blockColorArrayMap.erase(blockSelector);
  }
  std::vector<std::string> blockColorArrayVec;
  blockColorArrayVec.reserve(blockColorArrayMap.size() * 3);
  for (const auto& colorArray : blockColorArrayMap)
  {
    blockColorArrayVec.push_back(colorArray.first);
    blockColorArrayVec.push_back(std::to_string(colorArray.second.first));
    blockColorArrayVec.push_back(colorArray.second.second);
  }
  // the following trace is not needed because this function is not supposed to be publicly called
  // SM_SCOPED_TRACE(PropertiesModified)
  //   .arg("proxy", proxy)
  //   .arg("comment", " change block(s) color array");
  blockColorArrayNames->SetElements(blockColorArrayVec);
  proxy->UpdateProperty("BlockColorArrayNames");
}

//----------------------------------------------------------------------------
vtkSMColorMapEditorHelper::ColorArray vtkSMColorMapEditorHelper::GetColorArray(vtkSMProxy* proxy)
{
  const vtkSMPropertyHelper colorArray(proxy, "ColorArrayName");
  return std::make_pair(
    colorArray.GetInputArrayAssociation(), colorArray.GetInputArrayNameToProcess());
}

//----------------------------------------------------------------------------
std::vector<vtkSMColorMapEditorHelper::ColorArray> vtkSMColorMapEditorHelper::GetBlocksColorArrays(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return std::vector<ColorArray>();
  }
  const auto blockColorArray =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockColorArrayNames"));
  if (!blockColorArray)
  {
    vtkDebugWithObjectMacro(proxy, "No 'BlockColorArrayNames' property found.");
    return std::vector<ColorArray>(blockSelectors.size(), std::make_pair(-1, ""));
  }
  assert(blockColorArray->GetNumberOfElementsPerCommand() == 3);
  std::vector<ColorArray> colorArrays(blockSelectors.size(), std::make_pair(-1, ""));
  for (unsigned int i = 0; i < blockColorArray->GetNumberOfElements(); i += 3)
  {
    for (unsigned int j = 0; j < blockSelectors.size(); ++j)
    {
      if (blockColorArray->GetElement(i) == blockSelectors[j])
      {
        colorArrays[j] = std::make_pair(
          std::stoi(blockColorArray->GetElement(i + 1)), blockColorArray->GetElement(i + 2));
        break;
      }
    }
  }
  return colorArrays;
}

//----------------------------------------------------------------------------
std::map<vtkSMColorMapEditorHelper::ColorArray, std::vector<std::string>>
vtkSMColorMapEditorHelper::GetCommonColorArraysBlockSelectors(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return std::map<ColorArray, std::vector<std::string>>();
  }
  const std::vector<ColorArray> colorArrays =
    vtkSMColorMapEditorHelper::GetBlocksColorArrays(proxy, blockSelectors);
  std::map<ColorArray, std::vector<std::string>> commonColorArrays;
  for (size_t i = 0; i < blockSelectors.size(); ++i)
  {
    commonColorArrays[colorArrays[i]].push_back(blockSelectors[i]);
  }
  return commonColorArrays;
}
//----------------------------------------------------------------------------
std::vector<vtkSMColorMapEditorHelper::ColorArray>
vtkSMColorMapEditorHelper::GetSelectedColorArrays(vtkSMProxy* proxy)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    return vtkSMColorMapEditorHelper::GetBlocksColorArrays(
      proxy, vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy));
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    return { vtkSMColorMapEditorHelper::GetColorArray(proxy) };
  }
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkSMColorMapEditorHelper::GetUseSeparateColorMapProperty(vtkSMProxy* proxy)
{
  return proxy->GetProperty("UseSeparateColorMap");
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkSMColorMapEditorHelper::GetBlockUseSeparateColorMapProperty(vtkSMProxy* proxy)
{
  return proxy->GetProperty("BlockUseSeparateColorMaps");
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkSMColorMapEditorHelper::GetSelectedUseSeparateColorMapProperty(vtkSMProxy* proxy)
{
  return this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks
    ? vtkSMColorMapEditorHelper::GetBlockUseSeparateColorMapProperty(proxy)
    : vtkSMColorMapEditorHelper::GetUseSeparateColorMapProperty(proxy);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::IsUseSeparateColorMapValid(int useSeparateColorMap)
{
  return useSeparateColorMap == 0 || useSeparateColorMap == 1;
}
//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetUseSeparateColorMap(vtkSMProxy* proxy, bool use)
{
  SM_SCOPED_TRACE(PropertiesModified)
    .arg("proxy", proxy)
    .arg("comment", " change use separate color map");
  vtkSMPropertyHelper(proxy, "UseSeparateColorMap", true).Set(use ? 1 : 0);
  proxy->UpdateProperty("UseSeparateColorMap");
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetBlocksUseSeparateColorMap(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors, bool use)
{
  if (blockSelectors.empty())
  {
    return;
  }
  auto blockUseSeparateColorMaps =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockUseSeparateColorMaps"));
  if (!blockUseSeparateColorMaps)
  {
    vtkDebugWithObjectMacro(proxy, "No 'BlockUseSeparateColorMaps' property found.");
    return;
  }
  assert(blockUseSeparateColorMaps->GetNumberOfElementsPerCommand() == 2);
  std::map<std::string, bool> blockUseSeparateColorMapsMap;
  for (unsigned int i = 0; i < blockUseSeparateColorMaps->GetNumberOfElements(); i += 2)
  {
    blockUseSeparateColorMapsMap[blockUseSeparateColorMaps->GetElement(i)] =
      std::stoi(blockUseSeparateColorMaps->GetElement(i + 1));
  }
  for (const std::string& blockSelector : blockSelectors)
  {
    blockUseSeparateColorMapsMap[blockSelector] = use;
  }
  std::vector<std::string> blockUseSeparateColorMapsVec;
  blockUseSeparateColorMapsVec.reserve(blockUseSeparateColorMapsMap.size() * 2);
  for (const auto& blockUseSeparateColorMap : blockUseSeparateColorMapsMap)
  {
    blockUseSeparateColorMapsVec.push_back(blockUseSeparateColorMap.first);
    blockUseSeparateColorMapsVec.push_back(std::to_string(blockUseSeparateColorMap.second));
  }
  SM_SCOPED_TRACE(PropertiesModified)
    .arg("proxy", proxy)
    .arg("comment", " change block(s) use separate color map");
  blockUseSeparateColorMaps->SetElements(blockUseSeparateColorMapsVec);
  proxy->UpdateProperty("BlockUseSeparateColorMaps");
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::RemoveBlocksUseSeparateColorMaps(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return;
  }
  auto blockUseSeparateColorMaps =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockUseSeparateColorMaps"));
  if (!blockUseSeparateColorMaps)
  {
    vtkDebugWithObjectMacro(proxy, "No 'BlockUseSeparateColorMaps' property found.");
    return;
  }
  assert(blockUseSeparateColorMaps->GetNumberOfElementsPerCommand() == 2);
  std::map<std::string, std::string> blockUseSeparateColorMapsMap;
  for (unsigned int i = 0; i < blockUseSeparateColorMaps->GetNumberOfElements(); i += 2)
  {
    blockUseSeparateColorMapsMap[blockUseSeparateColorMaps->GetElement(i)] =
      blockUseSeparateColorMaps->GetElement(i + 1);
  }
  for (const std::string& blockSelector : blockSelectors)
  {
    blockUseSeparateColorMapsMap.erase(blockSelector);
  }
  std::vector<std::string> blockUseSeparateColorMapsVec;
  blockUseSeparateColorMapsVec.reserve(blockUseSeparateColorMapsMap.size() * 2);
  for (const auto& blockUseSeparateColorMap : blockUseSeparateColorMapsMap)
  {
    blockUseSeparateColorMapsVec.push_back(blockUseSeparateColorMap.first);
    blockUseSeparateColorMapsVec.push_back(blockUseSeparateColorMap.second);
  }
  SM_SCOPED_TRACE(PropertiesModified)
    .arg("proxy", proxy)
    .arg("comment", " change block(s) use separate color map");
  blockUseSeparateColorMaps->SetElements(blockUseSeparateColorMapsVec);
  proxy->UpdateProperty("BlockUseSeparateColorMaps");
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetSelectedUseSeparateColorMap(vtkSMProxy* proxy, bool use)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    vtkSMColorMapEditorHelper::SetBlocksUseSeparateColorMap(
      proxy, vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy), use);
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    vtkSMColorMapEditorHelper::SetUseSeparateColorMap(proxy, use);
  }
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::GetUseSeparateColorMap(vtkSMProxy* proxy)
{
  return vtkSMPropertyHelper(proxy, "UseSeparateColorMap", true).GetAsInt() != 0;
}

//----------------------------------------------------------------------------
std::vector<int> vtkSMColorMapEditorHelper::GetBlocksUseSeparateColorMaps(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return std::vector<int>();
  }
  const auto blockUseSeparateColorMaps =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockUseSeparateColorMaps"));
  if (!blockUseSeparateColorMaps)
  {
    vtkDebugWithObjectMacro(proxy, "No 'BlockUseSeparateColorMaps' property found.");
    return std::vector<int>(blockSelectors.size(), -1);
  }
  assert(blockUseSeparateColorMaps->GetNumberOfElementsPerCommand() == 2);
  std::vector<int> useSeparateColorMapsVec(blockSelectors.size(), -1);
  for (unsigned int i = 0; i < blockUseSeparateColorMaps->GetNumberOfElements(); i += 2)
  {
    for (unsigned int j = 0; j < blockSelectors.size(); ++j)
    {
      if (blockUseSeparateColorMaps->GetElement(i) == blockSelectors[j])
      {
        useSeparateColorMapsVec[j] = std::stoi(blockUseSeparateColorMaps->GetElement(i + 1));
        break;
      }
    }
  }
  return useSeparateColorMapsVec;
}

//----------------------------------------------------------------------------
std::vector<int> vtkSMColorMapEditorHelper::GetSelectedUseSeparateColorMaps(vtkSMProxy* proxy)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    return vtkSMColorMapEditorHelper::GetBlocksUseSeparateColorMaps(
      proxy, vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy));
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    return { vtkSMColorMapEditorHelper::GetUseSeparateColorMap(proxy) };
  }
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::GetAnySelectedUseSeparateColorMap(vtkSMProxy* proxy)
{
  const std::vector<int> useSeparateColorMaps =
    vtkSMColorMapEditorHelper::GetSelectedUseSeparateColorMaps(proxy);
  return std::any_of(useSeparateColorMaps.begin(), useSeparateColorMaps.end(),
    [](int useSeparateColorMap) { return useSeparateColorMap == 1; });
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::IsMapScalarsValid(int mapScalars)
{
  return mapScalars == 0 || mapScalars == 1;
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetMapScalars(vtkSMProxy* proxy, bool mapScalars)
{
  SM_SCOPED_TRACE(PropertiesModified).arg("proxy", proxy).arg("comment", " change map scalars");
  vtkSMPropertyHelper(proxy, "MapScalars", true).Set(mapScalars ? 1 : 0);
  proxy->UpdateProperty("MapScalars");
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetBlocksMapScalars(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors, bool mapScalars)
{
  if (blockSelectors.empty())
  {
    return;
  }
  auto blockMapScalars =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockMapScalars"));
  if (!blockMapScalars)
  {
    vtkDebugWithObjectMacro(proxy, "No 'BlockMapScalars' property found.");
    return;
  }
  assert(blockMapScalars->GetNumberOfElementsPerCommand() == 2);
  std::map<std::string, bool> blockMapScalarsMap;
  for (unsigned int i = 0; i < blockMapScalars->GetNumberOfElements(); i += 2)
  {
    blockMapScalarsMap[blockMapScalars->GetElement(i)] =
      std::stoi(blockMapScalars->GetElement(i + 1));
  }
  for (const std::string& blockSelector : blockSelectors)
  {
    blockMapScalarsMap[blockSelector] = mapScalars;
  }
  std::vector<std::string> blockMapScalarsVec;
  blockMapScalarsVec.reserve(blockMapScalarsMap.size() * 2);
  for (const auto& blockMapScalar : blockMapScalarsMap)
  {
    blockMapScalarsVec.push_back(blockMapScalar.first);
    blockMapScalarsVec.push_back(std::to_string(blockMapScalar.second));
  }
  SM_SCOPED_TRACE(PropertiesModified)
    .arg("proxy", proxy)
    .arg("comment", " change block(s) map scalars");
  blockMapScalars->SetElements(blockMapScalarsVec);
  proxy->UpdateProperty("BlockMapScalars");
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::RemoveBlocksMapScalars(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return;
  }
  auto blockMapScalars =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockMapScalars"));
  if (!blockMapScalars)
  {
    vtkDebugWithObjectMacro(proxy, "No 'BlockMapScalars' property found.");
    return;
  }
  assert(blockMapScalars->GetNumberOfElementsPerCommand() == 2);
  std::map<std::string, std::string> blockMapScalarsMap;
  for (unsigned int i = 0; i < blockMapScalars->GetNumberOfElements(); i += 2)
  {
    blockMapScalarsMap[blockMapScalars->GetElement(i)] = blockMapScalars->GetElement(i + 1);
  }
  for (const std::string& blockSelector : blockSelectors)
  {
    blockMapScalarsMap.erase(blockSelector);
  }
  std::vector<std::string> blockMapScalarsVec;
  blockMapScalarsVec.reserve(blockMapScalarsMap.size() * 2);
  for (const auto& blockMapScalar : blockMapScalarsMap)
  {
    blockMapScalarsVec.push_back(blockMapScalar.first);
    blockMapScalarsVec.push_back(blockMapScalar.second);
  }
  SM_SCOPED_TRACE(PropertiesModified)
    .arg("proxy", proxy)
    .arg("comment", " change block(s) map scalars");
  blockMapScalars->SetElements(blockMapScalarsVec);
  proxy->UpdateProperty("BlockMapScalars");
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetSelectedMapScalars(vtkSMProxy* proxy, bool mapScalars)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    vtkSMColorMapEditorHelper::SetBlocksMapScalars(
      proxy, vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy), mapScalars);
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    vtkSMColorMapEditorHelper::SetMapScalars(proxy, mapScalars);
  }
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::GetMapScalars(vtkSMProxy* proxy)
{
  return vtkSMPropertyHelper(proxy, "MapScalars", true).GetAsInt() != 0;
}

//----------------------------------------------------------------------------
std::vector<int> vtkSMColorMapEditorHelper::GetBlocksMapScalars(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return std::vector<int>();
  }
  const auto blockMapScalars =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockMapScalars"));
  if (!blockMapScalars)
  {
    vtkDebugWithObjectMacro(proxy, "No 'BlockMapScalars' property found.");
    return std::vector<int>(blockSelectors.size(), -1);
  }
  assert(blockMapScalars->GetNumberOfElementsPerCommand() == 2);
  std::vector<int> blockMapScalarsVec(blockSelectors.size(), -1);
  for (unsigned int i = 0; i < blockMapScalars->GetNumberOfElements(); i += 2)
  {
    for (unsigned int j = 0; j < blockSelectors.size(); ++j)
    {
      if (blockMapScalars->GetElement(i) == blockSelectors[j])
      {
        blockMapScalarsVec[j] = std::stoi(blockMapScalars->GetElement(i + 1));
        break;
      }
    }
  }
  return blockMapScalarsVec;
}

//----------------------------------------------------------------------------
std::vector<int> vtkSMColorMapEditorHelper::GetSelectedMapScalars(vtkSMProxy* proxy)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    return vtkSMColorMapEditorHelper::GetBlocksMapScalars(
      proxy, vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy));
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    return { vtkSMColorMapEditorHelper::GetMapScalars(proxy) };
  }
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::GetAnySelectedMapScalars(vtkSMProxy* proxy)
{
  const std::vector<int> mapScalars = vtkSMColorMapEditorHelper::GetSelectedMapScalars(proxy);
  return std::any_of(
    mapScalars.begin(), mapScalars.end(), [](int mapScalar) { return mapScalar == 1; });
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::IsInterpolateScalarsBeforeMappingValid(int interpolate)
{
  return interpolate == 0 || interpolate == 1;
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetInterpolateScalarsBeforeMapping(
  vtkSMProxy* proxy, bool interpolateScalars)
{
  SM_SCOPED_TRACE(PropertiesModified)
    .arg("proxy", proxy)
    .arg("comment", " change interpolate scalars before mapping");
  vtkSMPropertyHelper(proxy, "InterpolateScalarsBeforeMapping", true)
    .Set(interpolateScalars ? 1 : 0);
  proxy->UpdateProperty("InterpolateScalarsBeforeMapping");
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetBlocksInterpolateScalarsBeforeMapping(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors, bool interpolate)
{
  if (blockSelectors.empty())
  {
    return;
  }
  auto blockInterpolateScalarsBeforeMapping = vtkSMStringVectorProperty::SafeDownCast(
    proxy->GetProperty("BlockInterpolateScalarsBeforeMappings"));
  if (!blockInterpolateScalarsBeforeMapping)
  {
    vtkDebugWithObjectMacro(proxy, "No 'BlockInterpolateScalarsBeforeMappings' property found.");
    return;
  }
  assert(blockInterpolateScalarsBeforeMapping->GetNumberOfElementsPerCommand() == 2);
  std::map<std::string, bool> blockInterpolateScalarsBeforeMappingMap;
  for (unsigned int i = 0; i < blockInterpolateScalarsBeforeMapping->GetNumberOfElements(); i += 2)
  {
    blockInterpolateScalarsBeforeMappingMap[blockInterpolateScalarsBeforeMapping->GetElement(i)] =
      std::stoi(blockInterpolateScalarsBeforeMapping->GetElement(i + 1));
  }
  for (const std::string& blockSelector : blockSelectors)
  {
    blockInterpolateScalarsBeforeMappingMap[blockSelector] = interpolate;
  }
  std::vector<std::string> blockInterpolateScalarsBeforeMappingVec;
  blockInterpolateScalarsBeforeMappingVec.reserve(
    blockInterpolateScalarsBeforeMappingMap.size() * 2);
  for (const auto& blockInterpolate : blockInterpolateScalarsBeforeMappingMap)
  {
    blockInterpolateScalarsBeforeMappingVec.push_back(blockInterpolate.first);
    blockInterpolateScalarsBeforeMappingVec.push_back(std::to_string(blockInterpolate.second));
  }
  SM_SCOPED_TRACE(PropertiesModified)
    .arg("proxy", proxy)
    .arg("comment", " change block(s) interpolate scalars before mapping");
  blockInterpolateScalarsBeforeMapping->SetElements(blockInterpolateScalarsBeforeMappingVec);
  proxy->UpdateProperty("BlockInterpolateScalarsBeforeMappings");
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::RemoveBlocksInterpolateScalarsBeforeMappings(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return;
  }
  auto blockInterpolateScalarsBeforeMapping = vtkSMStringVectorProperty::SafeDownCast(
    proxy->GetProperty("BlockInterpolateScalarsBeforeMappings"));
  if (!blockInterpolateScalarsBeforeMapping)
  {
    vtkDebugWithObjectMacro(proxy, "No 'BlockInterpolateScalarsBeforeMappings' property found.");
    return;
  }
  assert(blockInterpolateScalarsBeforeMapping->GetNumberOfElementsPerCommand() == 2);
  std::map<std::string, bool> blockInterpolateScalarsBeforeMappingMap;
  for (unsigned int i = 0; i < blockInterpolateScalarsBeforeMapping->GetNumberOfElements(); i += 2)
  {
    blockInterpolateScalarsBeforeMappingMap[blockInterpolateScalarsBeforeMapping->GetElement(i)] =
      std::stoi(blockInterpolateScalarsBeforeMapping->GetElement(i + 1));
  }
  for (const std::string& blockSelector : blockSelectors)
  {
    blockInterpolateScalarsBeforeMappingMap.erase(blockSelector);
  }
  std::vector<std::string> blockInterpolateScalarsBeforeMappingVec;
  blockInterpolateScalarsBeforeMappingVec.reserve(
    blockInterpolateScalarsBeforeMappingMap.size() * 2);
  for (const auto& blockInterpolate : blockInterpolateScalarsBeforeMappingMap)
  {
    blockInterpolateScalarsBeforeMappingVec.push_back(blockInterpolate.first);
    blockInterpolateScalarsBeforeMappingVec.push_back(std::to_string(blockInterpolate.second));
  }
  SM_SCOPED_TRACE(PropertiesModified)
    .arg("proxy", proxy)
    .arg("comment", " change block(s) interpolate scalars before mapping");
  blockInterpolateScalarsBeforeMapping->SetElements(blockInterpolateScalarsBeforeMappingVec);
  proxy->UpdateProperty("BlockInterpolateScalarsBeforeMappings");
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetSelectedInterpolateScalarsBeforeMapping(
  vtkSMProxy* proxy, bool interpolateScalars)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    vtkSMColorMapEditorHelper::SetBlocksInterpolateScalarsBeforeMapping(
      proxy, vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy), interpolateScalars);
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    vtkSMColorMapEditorHelper::SetInterpolateScalarsBeforeMapping(proxy, interpolateScalars);
  }
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::GetInterpolateScalarsBeforeMapping(vtkSMProxy* proxy)
{
  return vtkSMPropertyHelper(proxy, "InterpolateScalarsBeforeMapping", true).GetAsInt() != 0;
}

//----------------------------------------------------------------------------
std::vector<int> vtkSMColorMapEditorHelper::GetBlocksInterpolateScalarsBeforeMappings(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return std::vector<int>();
  }
  const auto blockInterpolateScalarsBeforeMapping = vtkSMStringVectorProperty::SafeDownCast(
    proxy->GetProperty("BlockInterpolateScalarsBeforeMappings"));
  if (!blockInterpolateScalarsBeforeMapping)
  {
    vtkDebugWithObjectMacro(proxy, "No 'BlockInterpolateScalarsBeforeMappings' property found.");
    return std::vector<int>(blockSelectors.size(), -1);
  }
  assert(blockInterpolateScalarsBeforeMapping->GetNumberOfElementsPerCommand() == 2);
  std::vector<int> interpolateScalarsBeforeMappingVec(blockSelectors.size(), -1);
  for (unsigned int i = 0; i < blockInterpolateScalarsBeforeMapping->GetNumberOfElements(); i += 2)
  {
    for (unsigned int j = 0; j < blockSelectors.size(); ++j)
    {
      if (blockInterpolateScalarsBeforeMapping->GetElement(i) == blockSelectors[j])
      {
        interpolateScalarsBeforeMappingVec[j] =
          std::stoi(blockInterpolateScalarsBeforeMapping->GetElement(i + 1));
        break;
      }
    }
  }
  return interpolateScalarsBeforeMappingVec;
}

//----------------------------------------------------------------------------
std::vector<int> vtkSMColorMapEditorHelper::GetSelectedInterpolateScalarsBeforeMappings(
  vtkSMProxy* proxy)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    return vtkSMColorMapEditorHelper::GetBlocksInterpolateScalarsBeforeMappings(
      proxy, vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy));
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    return { vtkSMColorMapEditorHelper::GetInterpolateScalarsBeforeMapping(proxy) };
  }
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::GetAnySelectedInterpolateScalarsBeforeMapping(vtkSMProxy* proxy)
{
  const std::vector<int> interpolateScalarsBeforeMappings =
    vtkSMColorMapEditorHelper::GetSelectedInterpolateScalarsBeforeMappings(proxy);
  return std::any_of(interpolateScalarsBeforeMappings.begin(),
    interpolateScalarsBeforeMappings.end(),
    [](int interpolateScalarsBeforeMapping) { return interpolateScalarsBeforeMapping == 1; });
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::IsOpacityValid(double opacity)
{
  return opacity >= 0.0 && opacity <= 1.0;
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetOpacity(vtkSMProxy* proxy, double opacity)
{
  if (!vtkSMColorMapEditorHelper::IsOpacityValid(opacity))
  {
    vtkWarningWithObjectMacro(proxy, "Invalid opacity.");
    return;
  }
  SM_SCOPED_TRACE(PropertiesModified).arg("proxy", proxy).arg("comment", " change opacity");
  vtkSMPropertyHelper(proxy, "Opacity", true).Set(opacity);
  proxy->UpdateProperty("Opacity");
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetBlocksOpacity(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors, double opacity)
{
  if (blockSelectors.empty())
  {
    return;
  }
  if (!vtkSMColorMapEditorHelper::IsOpacityValid(opacity))
  {
    vtkWarningWithObjectMacro(proxy, "Invalid opacity.");
    return;
  }
  auto blockOpacities =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockOpacities"));
  if (!blockOpacities)
  {
    vtkDebugWithObjectMacro(proxy, "No 'BlockOpacities' property found.");
    return;
  }
  assert(blockOpacities->GetNumberOfElementsPerCommand() == 2);
  std::map<std::string, double> blockOpacitiesMap;
  for (unsigned int i = 0; i < blockOpacities->GetNumberOfElements(); i += 2)
  {
    blockOpacitiesMap[blockOpacities->GetElement(i)] = std::stod(blockOpacities->GetElement(i + 1));
  }
  for (const std::string& blockSelector : blockSelectors)
  {
    blockOpacitiesMap[blockSelector] = opacity;
  }
  std::vector<std::string> blockOpacitiesVec;
  blockOpacitiesVec.reserve(blockOpacitiesMap.size() * 2);
  for (const auto& blockOpacity : blockOpacitiesMap)
  {
    blockOpacitiesVec.push_back(blockOpacity.first);
    blockOpacitiesVec.push_back(std::to_string(blockOpacity.second));
  }
  SM_SCOPED_TRACE(PropertiesModified)
    .arg("proxy", proxy)
    .arg("comment", " change block(s) opacity");
  blockOpacities->SetElements(blockOpacitiesVec);
  proxy->UpdateProperty("BlockOpacities");
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::RemoveBlocksOpacities(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return;
  }
  auto blockOpacities =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockOpacities"));
  if (!blockOpacities)
  {
    vtkDebugWithObjectMacro(proxy, "No 'BlockOpacities' property found.");
    return;
  }
  assert(blockOpacities->GetNumberOfElementsPerCommand() == 2);
  std::map<std::string, double> blockOpacitiesMap;
  for (unsigned int i = 0; i < blockOpacities->GetNumberOfElements(); i += 2)
  {
    blockOpacitiesMap[blockOpacities->GetElement(i)] = std::stod(blockOpacities->GetElement(i + 1));
  }
  for (const std::string& blockSelector : blockSelectors)
  {
    blockOpacitiesMap.erase(blockSelector);
  }
  std::vector<std::string> blockOpacitiesVec;
  blockOpacitiesVec.reserve(blockOpacitiesMap.size() * 2);
  for (const auto& blockOpacity : blockOpacitiesMap)
  {
    blockOpacitiesVec.push_back(blockOpacity.first);
    blockOpacitiesVec.push_back(std::to_string(blockOpacity.second));
  }
  SM_SCOPED_TRACE(PropertiesModified)
    .arg("proxy", proxy)
    .arg("comment", " change block(s) opacity");
  blockOpacities->SetElements(blockOpacitiesVec);
  proxy->UpdateProperty("BlockOpacities");
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetSelectedOpacity(vtkSMProxy* proxy, double opacity)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    vtkSMColorMapEditorHelper::SetBlocksOpacity(
      proxy, vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy), opacity);
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    vtkSMColorMapEditorHelper::SetOpacity(proxy, opacity);
  }
}

//----------------------------------------------------------------------------
double vtkSMColorMapEditorHelper::GetOpacity(vtkSMProxy* proxy)
{
  return vtkSMPropertyHelper(proxy, "Opacity", true).GetAsDouble();
}

//----------------------------------------------------------------------------
std::vector<double> vtkSMColorMapEditorHelper::GetBlocksOpacities(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return std::vector<double>();
  }
  const auto blockOpacities =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockOpacities"));
  if (!blockOpacities)
  {
    vtkDebugWithObjectMacro(proxy, "No 'BlockOpacities' property found.");
    return std::vector<double>(blockSelectors.size(), VTK_DOUBLE_MAX);
  }
  assert(blockOpacities->GetNumberOfElementsPerCommand() == 2);
  std::vector<double> blockOpacitiesVec(blockSelectors.size(), VTK_DOUBLE_MAX);
  for (unsigned int i = 0; i < blockOpacities->GetNumberOfElements(); i += 2)
  {
    for (unsigned int j = 0; j < blockSelectors.size(); ++j)
    {
      if (blockOpacities->GetElement(i) == blockSelectors[j])
      {
        blockOpacitiesVec[j] = std::stod(blockOpacities->GetElement(i + 1));
        break;
      }
    }
  }
  return blockOpacitiesVec;
}

//----------------------------------------------------------------------------
std::vector<double> vtkSMColorMapEditorHelper::GetSelectedOpacities(vtkSMProxy* proxy)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    return vtkSMColorMapEditorHelper::GetBlocksOpacities(
      proxy, vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy));
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    return { vtkSMColorMapEditorHelper::GetOpacity(proxy) };
  }
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::ResetBlocksProperties(vtkSMProxy* proxy,
  const std::vector<std::string>& blockSelectors, const std::vector<std::string>& propertyNames)
{
  if (blockSelectors.empty())
  {
    return;
  }
  if (propertyNames.empty())
  {
    return;
  }
  std::vector<std::string> removedPropertyNames;
  if (std::find(propertyNames.begin(), propertyNames.end(), "BlockColors") != propertyNames.end())
  {
    vtkSMColorMapEditorHelper::RemoveBlocksColors(proxy, blockSelectors);
    removedPropertyNames.push_back("BlockColors");
  }
  if (std::find(propertyNames.begin(), propertyNames.end(), "BlockColorArrayNames") !=
      propertyNames.end() ||
    std::find(propertyNames.begin(), propertyNames.end(), "BlockLookupTables") !=
      propertyNames.end())
  {
    vtkSMColorMapEditorHelper::SetBlocksScalarColoring(proxy, blockSelectors, nullptr, -1);
    removedPropertyNames.push_back("BlockColorArrayNames");
    removedPropertyNames.push_back("BlockLookupTables");
  }
  if (std::find(propertyNames.begin(), propertyNames.end(), "BlockUseSeparateColorMaps") !=
    propertyNames.end())
  {
    vtkSMColorMapEditorHelper::RemoveBlocksUseSeparateColorMaps(proxy, blockSelectors);
    removedPropertyNames.push_back("BlockUseSeparateColorMaps");
    removedPropertyNames.push_back("BlockColorArrayNames");
  }
  if (std::find(propertyNames.begin(), propertyNames.end(), "BlockMapScalars") !=
    propertyNames.end())
  {
    vtkSMColorMapEditorHelper::RemoveBlocksMapScalars(proxy, blockSelectors);
    removedPropertyNames.push_back("BlockMapScalars");
  }
  if (std::find(propertyNames.begin(), propertyNames.end(),
        "BlockInterpolateScalarsBeforeMappings") != propertyNames.end())
  {
    vtkSMColorMapEditorHelper::RemoveBlocksInterpolateScalarsBeforeMappings(proxy, blockSelectors);
    removedPropertyNames.push_back("BlockInterpolateScalarsBeforeMappings");
  }
  if (std::find(propertyNames.begin(), propertyNames.end(), "BlockOpacities") !=
    propertyNames.end())
  {
    vtkSMColorMapEditorHelper::RemoveBlocksOpacities(proxy, blockSelectors);
    removedPropertyNames.push_back("BlockOpacities");
  }
  std::vector<std::string> notRemovedPropertyNames;
  for (const std::string& propertyName : propertyNames)
  {
    if (std::find(removedPropertyNames.begin(), removedPropertyNames.end(), propertyName) ==
      removedPropertyNames.end())
    {
      notRemovedPropertyNames.push_back(propertyName);
    }
  }
  for (const std::string& propertyName : notRemovedPropertyNames)
  {
    vtkWarningWithObjectMacro(proxy, "Property name: " << propertyName << " could not be removed");
  }
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetBlocksLookupTable(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors, vtkSMProxy* lut)
{
  if (blockSelectors.empty())
  {
    return;
  }
  auto blockColorArrayNames =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockColorArrayNames"));
  auto blockLUTs = vtkSMProxyProperty::SafeDownCast(proxy->GetProperty("BlockLookupTables"));
  if (!blockColorArrayNames || !blockLUTs)
  {
    vtkDebugWithObjectMacro(
      proxy, "No 'BlockColorArrayNames' or 'BlockLookupTables' properties found.");
    return;
  }
  std::map<std::string, vtkSMProxy*> blockLUTsMap;
  for (unsigned int i = 0; i < blockLUTs->GetNumberOfProxies(); ++i)
  {
    blockLUTsMap[blockColorArrayNames->GetElement(3 * i)] = blockLUTs->GetProxy(i);
  }
  for (const std::string& blockSelector : blockSelectors)
  {
    blockLUTsMap[blockSelector] = lut;
  }
  std::vector<vtkSMProxy*> blockLUTsVec;
  blockLUTsVec.reserve(blockLUTsMap.size());
  for (const auto& blockLUT : blockLUTsMap)
  {
    blockLUTsVec.push_back(blockLUT.second);
  }
  // the following trace is not needed because this function is not supposed to be publicly called
  // SM_SCOPED_TRACE(PropertiesModified).arg("proxy", proxy);
  blockLUTs->SetProxies(static_cast<unsigned int>(blockLUTsVec.size()), blockLUTsVec.data());
  proxy->UpdateProperty("BlockLookupTables");
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::RemoveBlocksLookupTables(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return;
  }
  auto blockColorArrayNames =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockColorArrayNames"));
  auto blockLUTs = vtkSMProxyProperty::SafeDownCast(proxy->GetProperty("BlockLookupTables"));
  if (!blockColorArrayNames || !blockLUTs)
  {
    vtkDebugWithObjectMacro(
      proxy, "No 'BlockColorArrayNames' or 'BlockLookupTables' properties found.");
    return;
  }
  std::map<std::string, vtkSMProxy*> blockLUTsMap;
  for (unsigned int i = 0; i < blockLUTs->GetNumberOfProxies(); ++i)
  {
    blockLUTsMap[blockColorArrayNames->GetElement(3 * i)] = blockLUTs->GetProxy(i);
  }
  for (const std::string& blockSelector : blockSelectors)
  {
    blockLUTsMap.erase(blockSelector);
  }
  std::vector<vtkSMProxy*> blockLUTsVec;
  blockLUTsVec.reserve(blockLUTsMap.size());
  for (const auto& blockLUT : blockLUTsMap)
  {
    blockLUTsVec.push_back(blockLUT.second);
  }
  // the following trace is not needed because this function is not supposed to be publicly called
  // SM_SCOPED_TRACE(PropertiesModified).arg("proxy", proxy);
  blockLUTs->SetProxies(static_cast<unsigned int>(blockLUTsVec.size()), blockLUTsVec.data());
  proxy->UpdateProperty("BlockLookupTables");
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMColorMapEditorHelper::GetLookupTable(vtkSMProxy* proxy)
{
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
vtkSMProxy* vtkSMColorMapEditorHelper::GetLookupTable(vtkSMProxy* proxy, vtkSMProxy* view)
{
  if (!view)
  {
    return nullptr;
  }
  return vtkSMColorMapEditorHelper::GetLookupTable(proxy);
}

//----------------------------------------------------------------------------
std::vector<vtkSMProxy*> vtkSMColorMapEditorHelper::GetBlocksLookupTables(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return std::vector<vtkSMProxy*>();
  }
  const auto blockColorArrayNames =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("BlockColorArrayNames"));
  const auto blockLUTs = vtkSMProxyProperty::SafeDownCast(proxy->GetProperty("BlockLookupTables"));
  if (!blockColorArrayNames || !blockLUTs)
  {
    vtkDebugWithObjectMacro(
      proxy, "No 'BlockColorArrayNames' or 'BlockLookupTables' properties found.");
    return std::vector<vtkSMProxy*>(blockSelectors.size(), nullptr);
  }
  std::vector<vtkSMProxy*> blockLUTsVec(blockSelectors.size(), nullptr);
  for (unsigned int i = 0; i < blockLUTs->GetNumberOfProxies(); ++i)
  {
    for (size_t j = 0; j < blockSelectors.size(); ++j)
    {
      if (blockColorArrayNames->GetElement(3 * i) == blockSelectors[j])
      {
        blockLUTsVec[j] = blockLUTs->GetProxy(i);
        break;
      }
    }
  }
  return blockLUTsVec;
}

//----------------------------------------------------------------------------
std::vector<vtkSMProxy*> vtkSMColorMapEditorHelper::GetBlocksLookupTables(
  vtkSMProxy* proxy, vtkSMProxy* view, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return std::vector<vtkSMProxy*>();
  }
  if (!view)
  {
    return std::vector<vtkSMProxy*>(blockSelectors.size(), nullptr);
  }
  return vtkSMColorMapEditorHelper::GetBlocksLookupTables(proxy, blockSelectors);
}

//----------------------------------------------------------------------------
std::vector<vtkSMProxy*> vtkSMColorMapEditorHelper::GetSelectedLookupTables(vtkSMProxy* proxy)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    return vtkSMColorMapEditorHelper::GetBlocksLookupTables(
      proxy, vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy));
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    vtkSMProxy* lut = vtkSMColorMapEditorHelper::GetLookupTable(proxy);
    if (!lut)
    {
      return {};
    }
    return { lut };
  }
}

//----------------------------------------------------------------------------
std::vector<vtkSMProxy*> vtkSMColorMapEditorHelper::GetSelectedLookupTables(
  vtkSMProxy* proxy, vtkSMProxy* view)
{
  if (!view)
  {
    return {};
  }
  return vtkSMColorMapEditorHelper::GetSelectedLookupTables(proxy, view);
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
  vtkSMProxy* lut = lutPropertyHelper.GetAsProxy();
  auto sbProxy = vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
    vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lut, view));
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
std::vector<int> vtkSMColorMapEditorHelper::IsBlocksScalarBarStickyVisible(
  vtkSMProxy* proxy, vtkSMProxy* view, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return std::vector<int>();
  }
  if (!view)
  {
    return std::vector<int>(blockSelectors.size(), -1);
  }
  const std::vector<vtkSMProxy*> blockLuts =
    vtkSMColorMapEditorHelper::GetBlocksLookupTables(proxy, blockSelectors);

  std::vector<int> blockStickyVisibleVec(blockSelectors.size());
  for (unsigned int i = 0; i < blockSelectors.size(); ++i)
  {
    if (vtkSMProxy* blockLut = blockLuts[i])
    {
      if (auto sbProxy = vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
            vtkSMTransferFunctionProxy::FindScalarBarRepresentation(blockLut, view)))
      {
        const vtkSMPropertyHelper sbsvPropertyHelper(sbProxy->GetProperty("StickyVisible"));
        blockStickyVisibleVec[i] = sbsvPropertyHelper.GetNumberOfElements()
          ? vtkSMPropertyHelper(sbProxy, "StickyVisible").GetAsInt()
          : -1;
      }
      else
      {
        blockStickyVisibleVec[i] = -1;
      }
    }
    else
    {
      blockStickyVisibleVec[i] = -1;
    }
  }
  return blockStickyVisibleVec;
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
  vtkSMProxy* lut = usingScalarBarColoring ? lutPropertyHelper.GetAsProxy()
                                           : vtkSMColorMapEditorHelper::GetLastLookupTable(proxy);

  if (!lut)
  {
    return false;
  }

  auto sbProxy = vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
    vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lut, view));

  if (!sbProxy)
  {
    return false;
  }

  vtkNew<vtkSMTransferFunctionManager> mgr;
  vtkSMProxy* sbSMProxy = mgr->GetScalarBarRepresentation(lut, view);

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
    const int component = vtkSMPropertyHelper(lut, "VectorMode").GetAsInt() != 0
      ? vtkSMPropertyHelper(lut, "VectorComponent").GetAsInt()
      : -1;
    sbProxy->GetRange(updatedRange, component);
    minRangePropHelper.Set(updatedRange[0]);
    maxRangePropHelper.Set(updatedRange[1]);
  }
  sbSMProxy->UpdateVTKObjects();

  vtkSMColorMapEditorHelper::SetLastLookupTable(proxy, usingScalarBarColoring ? lut : nullptr);
  return true;
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMColorMapEditorHelper::UpdateBlocksScalarBarRange(
  vtkSMProxy* proxy, vtkSMProxy* view, bool deleteRange)
{
  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  if (!repr)
  {
    return {};
  }
  // get the selectors of the colored blocks
  const std::vector<std::string> blockSelectors =
    vtkSMColorMapEditorHelper::GetColorArraysBlockSelectors(proxy);

  const std::vector<vtkTypeBool> blocksUsingScalarColoring =
    vtkSMColorMapEditorHelper::GetBlocksUsingScalarColoring(repr, blockSelectors);

  std::vector<vtkSMProxy*> blockLuts =
    vtkSMColorMapEditorHelper::GetBlocksLookupTables(proxy, blockSelectors);
  std::vector<vtkSMProxy*> lastblockLuts =
    vtkSMColorMapEditorHelper::GetLastBlocksLookupTables(repr, blockSelectors);
  for (unsigned int i = 0; i < blockSelectors.size(); ++i)
  {
    blockLuts[i] = blocksUsingScalarColoring[i] ? blockLuts[i] : lastblockLuts[i];
  }
  vtkNew<vtkSMTransferFunctionManager> mgr;
  std::vector<vtkTypeBool> updatedRanges(blockSelectors.size(), false);
  for (unsigned int i = 0; i < blockSelectors.size(); ++i)
  {
    if (!blockLuts[i])
    {
      updatedRanges[i] = false;
      continue;
    }

    auto sbProxy = vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
      vtkSMTransferFunctionProxy::FindScalarBarRepresentation(blockLuts[i], view));
    if (!sbProxy)
    {
      updatedRanges[i] = false;
      continue;
    }

    vtkSMProxy* sbSMProxy = mgr->GetScalarBarRepresentation(blockLuts[i], view);

    vtkSMProperty* maxRangeProp = sbSMProxy->GetProperty("DataRangeMax");
    vtkSMProperty* minRangeProp = sbSMProxy->GetProperty("DataRangeMin");

    if (minRangeProp && maxRangeProp)
    {
      vtkSMPropertyHelper minRangePropHelper(minRangeProp);
      vtkSMPropertyHelper maxRangePropHelper(maxRangeProp);

      // We remove the range that was potentially previously stored
      sbProxy->RemoveBlockRange(repr, blockSelectors[i]);

      // If we do not want to delete this range, then we update it with its potential new value.
      if (!deleteRange)
      {
        sbProxy->AddBlockRange(repr, blockSelectors[i]);
      }

      double updatedRange[2];
      const int component = vtkSMPropertyHelper(blockLuts[i], "VectorMode").GetAsInt() != 0
        ? vtkSMPropertyHelper(blockLuts[i], "VectorComponent").GetAsInt()
        : -1;
      sbProxy->GetRange(updatedRange, component);
      minRangePropHelper.Set(updatedRange[0]);
      maxRangePropHelper.Set(updatedRange[1]);
    }
    sbSMProxy->UpdateVTKObjects();
    vtkSMColorMapEditorHelper::SetLastBlockLookupTable(
      repr, blockSelectors[i], blocksUsingScalarColoring[i] ? blockLuts[i] : nullptr);
    updatedRanges[i] = true;
  }
  return updatedRanges;
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

  vtkSMProxy* lut = vtkSMColorMapEditorHelper::GetLookupTable(proxy, view);
  if (!lut)
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
  vtkSMProxy* lastLut = vtkSMColorMapEditorHelper::GetLastLookupTable(proxy);
  if (lastLut && lut != lastLut)
  {
    if (auto lastSBProxy = vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
          vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lastLut, view)))
    {
      lastSBProxy->RemoveRange(repr);
    }
  }

  auto sbProxy = vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
    vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lut, view));

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
  vtkSMProxy* sbSMProxy = mgr->GetScalarBarRepresentation(lut, view);
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
std::vector<vtkTypeBool> vtkSMColorMapEditorHelper::SetBlocksScalarBarVisibility(
  vtkSMProxy* proxy, vtkSMProxy* view, const std::vector<std::string>& blockSelectors, bool visible)
{
  if (blockSelectors.empty())
  {
    return std::vector<vtkTypeBool>();
  }
  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  if (!repr || !view)
  {
    return std::vector<vtkTypeBool>(blockSelectors.size(), false);
  }

  const std::vector<vtkSMProxy*> blockLuts =
    vtkSMColorMapEditorHelper::GetBlocksLookupTables(proxy, view, blockSelectors);
  if (std::all_of(blockLuts.begin(), blockLuts.end(), [](vtkSMProxy* lut) { return !lut; }))
  {
    vtkGenericWarningMacro("Failed to determine the LookupTables being used.");
    return std::vector<vtkTypeBool>(blockSelectors.size(), false);
  }

  SM_SCOPED_TRACE(CallMethod)
    .arg(repr)
    .arg("SetBlocksScalarBarVisibility")
    .arg(view)
    .arg(blockSelectors)
    .arg(visible)
    .arg("comment",
      visible ? "show block(s) color bar/color legend" : "hide block color bar/color legend");

  const std::vector<vtkSMProxy*> lastBlockLuts =
    vtkSMColorMapEditorHelper::GetLastBlocksLookupTables(repr, blockSelectors);
  const std::vector<vtkTypeBool> blocksUsingScalarColoring =
    vtkSMColorMapEditorHelper::GetBlocksUsingScalarColoring(repr, blockSelectors);
  vtkNew<vtkSMTransferFunctionManager> mgr;
  const std::vector<ColorArray> blockColorArrayNames =
    vtkSMColorMapEditorHelper::GetBlocksColorArrays(proxy, blockSelectors);

  std::vector<vtkTypeBool> updatedRanges(blockSelectors.size(), false);
  for (size_t i = 0; i < blockSelectors.size(); ++i)
  {
    const std::string& blockSelector = blockSelectors[i];
    vtkSMProxy* blockLut = blockLuts[i];
    vtkSMProxy* lastBlockLut = lastBlockLuts[i];
    // If the lut proxy changed, we need to remove ourself (representation proxy)
    // from the scalar bar widget that we used to be linked to.
    if (lastBlockLut && blockLut != lastBlockLut)
    {
      if (auto lastSBProxy = vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
            vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lastBlockLut, view)))
      {
        lastSBProxy->RemoveBlockRange(repr, blockSelector);
      }
    }
    auto sbProxy = vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
      vtkSMTransferFunctionProxy::FindScalarBarRepresentation(blockLut, view));

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
      updatedRanges[i] = true;
      continue;
    }

    if (!blocksUsingScalarColoring[i])
    {
      updatedRanges[i] = false;
      continue;
    }

    vtkSMProxy* sbSMProxy = mgr->GetScalarBarRepresentation(blockLut, view);
    if (!sbSMProxy)
    {
      vtkGenericWarningMacro("Failed to locate/create ScalarBar representation.");
      updatedRanges[i] = false;
      continue;
    }

    vtkSMPropertyHelper(sbSMProxy, "Enabled").Set(1);
    vtkSMPropertyHelper(sbSMProxy, "Visibility").Set(1);

    vtkSMProperty* titleProp = sbSMProxy->GetProperty("Title");
    vtkSMProperty* compProp = sbSMProxy->GetProperty("ComponentTitle");

    if (titleProp && compProp && titleProp->IsValueDefault() && compProp->IsValueDefault())
    {
      vtkSMSettings* settings = vtkSMSettings::GetInstance();

      const ColorArray& blockAttributeTypeAndName = blockColorArrayNames[i];
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
      vtkPVArrayInformation* blockArrayInfo =
        vtkSMColorMapEditorHelper::GetBlockArrayInformationForColorArray(repr, blockSelector);
      vtkSMScalarBarWidgetRepresentationProxy::UpdateComponentTitle(sbSMProxy, blockArrayInfo);
    }
    vtkSMScalarBarWidgetRepresentationProxy::PlaceInView(sbSMProxy, view);
    sbSMProxy->UpdateVTKObjects();
    updatedRanges[i] = true;
  }
  return updatedRanges;
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMColorMapEditorHelper::SetSelectedScalarBarVisibility(
  vtkSMProxy* proxy, vtkSMProxy* view, bool visible)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    return vtkSMColorMapEditorHelper::SetBlocksScalarBarVisibility(
      proxy, view, vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy), visible);
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    return { vtkSMColorMapEditorHelper::SetScalarBarVisibility(proxy, view, visible) };
  }
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::HideScalarBarIfNotNeeded(vtkSMProxy* proxy, vtkSMProxy* view)
{
  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  vtkSMProperty* lutProperty = proxy->GetProperty("LookupTable");
  if (!view || !repr || !lutProperty)
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
    .arg("comment", " hide scalars not actively used");

  vtkSMProxy* lut = lutPropertyHelper.GetAsProxy();
  vtkNew<vtkSMTransferFunctionManager> tmgr;
  return tmgr->HideScalarBarIfNotNeeded(lut, view);
}

//----------------------------------------------------------------------------
bool vtkSMColorMapEditorHelper::HideBlocksScalarBarIfNotNeeded(vtkSMProxy* proxy, vtkSMProxy* view)
{
  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  auto blockLUTs = vtkSMProxyProperty::SafeDownCast(repr->GetProperty("BlockLookupTables"));
  if (!view || !repr || !blockLUTs || blockLUTs->GetNumberOfProxies() == 0)
  {
    return false;
  }

  SM_SCOPED_TRACE(CallMethod)
    .arg(proxy)
    .arg("HideBlocksScalarBarIfNotNeeded")
    .arg(view)
    .arg("comment", " hide blocks scalars not actively used");

  vtkNew<vtkSMTransferFunctionManager> tmgr;
  bool result = false;
  for (unsigned int i = 0; i < blockLUTs->GetNumberOfProxies(); ++i)
  {
    vtkSMProxy* blockLut = blockLUTs->GetProxy(i);
    if (!blockLut)
    {
      continue;
    }
    result |= tmgr->HideScalarBarIfNotNeeded(blockLut, view);
  }
  return result;
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

  vtkSMProxy* lut = lutPropertyHelper.GetAsProxy();
  return vtkSMTransferFunctionProxy::IsScalarBarVisible(lut, view);
}

//----------------------------------------------------------------------------
std::vector<vtkTypeBool> vtkSMColorMapEditorHelper::IsBlocksScalarBarVisible(
  vtkSMProxy* proxy, vtkSMProxy* view, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return std::vector<vtkTypeBool>();
  }
  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  std::vector<vtkSMProxy*> blockLuts =
    vtkSMColorMapEditorHelper::GetBlocksLookupTables(proxy, view, blockSelectors);
  if (!repr)
  {
    return std::vector<vtkTypeBool>(blockSelectors.size(), false);
  }
  std::vector<vtkTypeBool> blockScalarBarVisibleVec(blockSelectors.size(), false);
  for (unsigned int i = 0; i < blockSelectors.size(); ++i)
  {
    vtkSMProxy* blockLut = blockLuts[i];
    if (blockLut)
    {
      blockScalarBarVisibleVec[i] = vtkSMTransferFunctionProxy::IsScalarBarVisible(blockLut, view);
    }
  }
  return blockScalarBarVisibleVec;
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
std::vector<vtkPVArrayInformation*>
vtkSMColorMapEditorHelper::GetBlocksArrayInformationForColorArray(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return std::vector<vtkPVArrayInformation*>();
  }
  const std::vector<vtkTypeBool> blocksUsingScalarColoring =
    vtkSMColorMapEditorHelper::GetBlocksUsingScalarColoring(proxy, blockSelectors);

  std::vector<vtkPVArrayInformation*> blockArrayInfos(blockSelectors.size(), nullptr);
  for (unsigned int i = 0; i < blockSelectors.size(); ++i)
  {
    const std::string& blockSelector = blockSelectors[i];
    if (!blocksUsingScalarColoring[i])
    {
      blockArrayInfos[i] = nullptr;
    }
    else
    {
      auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
      // now, determine a name for it if possible.
      const ColorArray attributeTypeAndArrayName =
        vtkSMColorMapEditorHelper::GetBlockColorArray(repr, blockSelector);
      const int attributeType = attributeTypeAndArrayName.first;
      const std::string arrayName = attributeTypeAndArrayName.second;

      const vtkSMPropertyHelper inputHelper(repr->GetProperty("Input"));
      vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
      const int port = inputHelper.GetOutputPort();
      if (!inputProxy || !inputProxy->GetOutputPort(port))
      {
        vtkWarningWithObjectMacro(repr, "No input present. Cannot determine data ranges.");
        blockArrayInfos[i] = nullptr;
        continue;
      }
      else // inputProxy && inputProxy->GetOutputPort(port)
      {
        vtkPVDataInformation* blockDataInfo =
          inputProxy->GetOutputPort(port)->GetSubsetDataInformation(blockSelector.c_str(),
            vtkSMPropertyHelper(repr->GetProperty("Assembly"), true).GetAsString());

        vtkPVArrayInformation* blockArrayInfo = nullptr;
        blockArrayInfo = blockDataInfo->GetArrayInformation(arrayName.c_str(), attributeType);
        if (blockArrayInfo)
        {
          blockArrayInfos[i] = blockArrayInfo;
        }

        if (attributeType == vtkDataObject::POINT_THEN_CELL)
        {
          // Try points...
          blockArrayInfo =
            blockDataInfo->GetArrayInformation(arrayName.c_str(), vtkDataObject::POINT);
          if (blockArrayInfo)
          {
            blockArrayInfos[i] = blockArrayInfo;
          }

          // ... then cells
          blockArrayInfo =
            blockDataInfo->GetArrayInformation(arrayName.c_str(), vtkDataObject::CELL);
          if (blockArrayInfo)
          {
            blockArrayInfos[i] = blockArrayInfo;
          }
        }
      }
      // In GetArrayInformationForColorArray we also check for checkRepresentedData,
      // but we can not do that for blocks, so we skip it.
    }
  }
  return blockArrayInfos;
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
std::vector<vtkPVProminentValuesInformation*>
vtkSMColorMapEditorHelper::GetBlocksProminentValuesInformationForColorArray(vtkSMProxy* proxy,
  const std::vector<std::string>& blockSelectors, double uncertaintyAllowed, double fraction,
  bool force)
{
  if (blockSelectors.empty())
  {
    return std::vector<vtkPVProminentValuesInformation*>();
  }
  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);

  const std::vector<vtkPVArrayInformation*> blockArrayInfos =
    vtkSMColorMapEditorHelper::GetBlocksArrayInformationForColorArray(proxy, blockSelectors);
  const std::vector<ColorArray> blockColorArrays =
    vtkSMColorMapEditorHelper::GetBlocksColorArrays(proxy, blockSelectors);

  std::vector<vtkPVProminentValuesInformation*> blockProminentValuesInfos(
    blockSelectors.size(), nullptr);
  for (unsigned int i = 0; i < blockSelectors.size(); ++i)
  {
    if (vtkPVArrayInformation* blockArrayInfo = blockArrayInfos[i])
    {
      const auto attributeTypeAndArrayName = blockColorArrays[i];
      blockProminentValuesInfos[i] = repr->GetBlockProminentValuesInformation(blockSelectors[i],
        vtkSMPropertyHelper(repr->GetProperty("Assembly"), true).GetAsString(),
        blockArrayInfo->GetName(), attributeTypeAndArrayName.first,
        blockArrayInfo->GetNumberOfComponents(), uncertaintyAllowed, fraction, force);
    }
    else
    {
      blockProminentValuesInfos[i] = nullptr;
    }
  }
  return blockProminentValuesInfos;
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

  vtkSMProxy* lut = lutPropertyHelper.GetAsProxy();
  vtkNew<vtkSMTransferFunctionManager> mgr;
  vtkSMProxy* sbProxy = mgr->GetScalarBarRepresentation(lut, view);
  if (!sbProxy)
  {
    vtkGenericWarningMacro("Failed to locate/create ScalarBar representation.");
    return -1;
  }

  sbProxy->UpdatePropertyInformation();
  return vtkSMPropertyHelper(sbProxy, "EstimatedNumberOfAnnotations").GetAsInt();
}

//----------------------------------------------------------------------------
std::vector<int> vtkSMColorMapEditorHelper::GetBlocksEstimatedNumberOfAnnotationsOnScalarBars(
  vtkSMProxy* proxy, vtkSMProxy* view, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return std::vector<int>();
  }
  auto repr = vtkSMRepresentationProxy::SafeDownCast(proxy);
  if (!repr || !view)
  {
    return std::vector<int>(blockSelectors.size(), -1);
  }
  const std::vector<vtkTypeBool> blocksUsingScalarColoring =
    vtkSMColorMapEditorHelper::GetBlocksUsingScalarColoring(proxy, blockSelectors);
  const std::vector<vtkSMProxy*> blockLuts =
    vtkSMColorMapEditorHelper::GetBlocksLookupTables(proxy, blockSelectors);

  std::vector<int> estimatedNumberOfAnnotationsVec(blockSelectors.size());
  for (unsigned int i = 0; i < blockSelectors.size(); ++i)
  {
    if (!blocksUsingScalarColoring[i])
    {
      estimatedNumberOfAnnotationsVec[i] = 0;
    }
    else
    {
      if (vtkSMProxy* blockLut = blockLuts[i])
      {
        vtkNew<vtkSMTransferFunctionManager> mgr;
        vtkSMProxy* sbProxy = mgr->GetScalarBarRepresentation(blockLut, view);
        if (!sbProxy)
        {
          vtkGenericWarningMacro("Failed to locate/create ScalarBar representation.");
          estimatedNumberOfAnnotationsVec[i] = -1;
        }
        else
        {
          sbProxy->UpdatePropertyInformation();
          estimatedNumberOfAnnotationsVec[i] =
            vtkSMPropertyHelper(sbProxy, "EstimatedNumberOfAnnotations").GetAsInt();
        }
      }
      else
      {
        estimatedNumberOfAnnotationsVec[i] = -1;
      }
    }
  }
  return estimatedNumberOfAnnotationsVec;
}

//----------------------------------------------------------------------------
std::vector<int> vtkSMColorMapEditorHelper::GetSelectedEstimatedNumberOfAnnotationsOnScalarBars(
  vtkSMProxy* proxy, vtkSMProxy* view)
{
  if (this->SelectedPropertiesType == SelectedPropertiesTypes::Blocks)
  {
    return vtkSMColorMapEditorHelper::GetBlocksEstimatedNumberOfAnnotationsOnScalarBars(
      proxy, view, vtkSMColorMapEditorHelper::GetSelectedBlockSelectors(proxy));
  }
  else // this->SelectedPropertiesType == SelectedPropertiesTypes::Representation
  {
    return { vtkSMColorMapEditorHelper::GetEstimatedNumberOfAnnotationsOnScalarBar(proxy, view) };
  }
}

//----------------------------------------------------------------------------
int vtkSMColorMapEditorHelper::GetAnySelectedEstimatedNumberOfAnnotationsOnScalarBar(
  vtkSMProxy* proxy, vtkSMProxy* view)
{
  const std::vector<int> annotations =
    vtkSMColorMapEditorHelper::GetSelectedEstimatedNumberOfAnnotationsOnScalarBars(proxy, view);
  auto findResult = std::find_if(
    annotations.begin(), annotations.end(), [](int annotation) { return annotation >= 0; });
  return findResult != annotations.end() ? *findResult : -1;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMColorMapEditorHelper::GetLastLookupTable(vtkSMProxy* proxy)
{
  auto repr = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
  return repr ? repr->GetLastLookupTable() : nullptr;
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetLastLookupTable(vtkSMProxy* proxy, vtkSMProxy* lut)
{
  auto repr = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
  if (repr)
  {
    repr->SetLastLookupTable(lut);
  }
}

//----------------------------------------------------------------------------
std::vector<vtkSMProxy*> vtkSMColorMapEditorHelper::GetLastBlocksLookupTables(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors)
{
  if (blockSelectors.empty())
  {
    return std::vector<vtkSMProxy*>();
  }
  auto repr = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
  return repr ? repr->GetLastBlocksLookupTables(blockSelectors)
              : std::vector<vtkSMProxy*>(blockSelectors.size(), nullptr);
}

//----------------------------------------------------------------------------
void vtkSMColorMapEditorHelper::SetLastBlocksLookupTable(
  vtkSMProxy* proxy, const std::vector<std::string>& blockSelectors, vtkSMProxy* lut)
{
  if (blockSelectors.empty())
  {
    return;
  }
  auto repr = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
  if (repr)
  {
    repr->SetLastBlocksLookupTable(blockSelectors, lut);
  }
}
