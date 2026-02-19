// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMChartSeriesSelectionDomain.h"

#include "vtkChartRepresentation.h"
#include "vtkCommand.h"
#include "vtkDataAssembly.h"
#include "vtkDataObject.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVGeneralSettings.h"
#include "vtkPVRepresentedArrayListSettings.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTransferFunctionPresets.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkStringList.h"

#include <vtk_jsoncpp.h>
#include <vtksys/RegularExpression.hxx>

#include <cassert>
#include <memory>
#include <set>
#include <sstream>
#include <vector>

vtkStandardNewMacro(vtkSMChartSeriesSelectionDomain);
namespace
{
// match string like: "ACCL (0)" or "VEL (1)"
vtksys::RegularExpression PotentialComponentNameRe(".*\\([0-9]+\\)");
}

class vtkSMChartSeriesSelectionDomain::vtkInternals
{
  unsigned int ColorCounter;

public:
  std::map<std::string, bool> VisibilityOverrides;

  vtkInternals()
    : ColorCounter(0)
  {
  }

  void GetNextColor(const char* presetName, double rgb[3])
  {
    auto presets = vtkSMTransferFunctionPresets::GetInstance();
    const char* name = presetName ? presetName : "";
    if (!presets->HasPreset(name))
    {
      vtkWarningWithObjectMacro(
        nullptr, "No preset with this name (" << name << "). Fall back to Spectrum.");
      name = "Spectrum";
    }
    const Json::Value& preset = presets->GetFirstPresetWithName(name);
    const Json::Value& indexedColors = preset["IndexedColors"];
    auto nbOfColors = indexedColors.size() / 3;
    int idx = this->ColorCounter % nbOfColors;
    rgb[0] = indexedColors[3 * idx].asDouble();
    rgb[1] = indexedColors[3 * idx + 1].asDouble();
    rgb[2] = indexedColors[3 * idx + 2].asDouble();
    this->ColorCounter++;
  }
};

//----------------------------------------------------------------------------
vtkSMChartSeriesSelectionDomain::vtkSMChartSeriesSelectionDomain()
  : Internals(new vtkSMChartSeriesSelectionDomain::vtkInternals())
{
  this->DefaultMode = vtkSMChartSeriesSelectionDomain::UNDEFINED;
  this->DefaultValue = nullptr;
  this->SetDefaultValue("");
  this->FlattenTable = true;
  this->HidePartialArrays = true;

  this->AddObserver(
    vtkCommand::DomainModifiedEvent, this, &vtkSMChartSeriesSelectionDomain::OnDomainModified);
}

//----------------------------------------------------------------------------
vtkSMChartSeriesSelectionDomain::~vtkSMChartSeriesSelectionDomain()
{
  this->SetDefaultValue(nullptr);
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMChartSeriesSelectionDomain::GetInputInformation()
{
  vtkSMProperty* inputProperty = this->GetRequiredProperty("Input");
  assert(inputProperty);

  vtkSMUncheckedPropertyHelper helper(inputProperty);
  if (helper.GetNumberOfElements() > 0)
  {
    vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(helper.GetAsProxy(0));
    if (sp)
    {
      return sp->GetDataInformation(helper.GetOutputPort());
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------
int vtkSMChartSeriesSelectionDomain::ReadXMLAttributes(
  vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }

  const char* default_mode = element->GetAttribute("default_mode");
  if (default_mode)
  {
    if (strcmp(default_mode, "visibility") == 0)
    {
      this->DefaultMode = vtkSMChartSeriesSelectionDomain::VISIBILITY;
    }
    else if (strcmp(default_mode, "label") == 0)
    {
      this->DefaultMode = vtkSMChartSeriesSelectionDomain::LABEL;
    }
    else if (strcmp(default_mode, "color") == 0)
    {
      this->DefaultMode = vtkSMChartSeriesSelectionDomain::COLOR;
    }
    else if (strcmp(default_mode, "value") == 0)
    {
      this->DefaultMode = vtkSMChartSeriesSelectionDomain::VALUE;
      if (element->GetAttribute("default_value"))
      {
        this->SetDefaultValue(element->GetAttribute("default_value"));
      }
    }
  }

  const char* flatten_table = element->GetAttribute("flatten_table");
  if (flatten_table)
  {
    this->FlattenTable = (strcmp(default_mode, "true") == 0);
  }
  int hide_partial_arrays;
  if (element->GetScalarAttribute("hide_partial_arrays", &hide_partial_arrays))
  {
    this->HidePartialArrays = (hide_partial_arrays == 1);
  }
  return 1;
}

namespace
{
//----------------------------------------------------------------------------
// Convert Keys from std::map<Key. Value> as std::vector<T>.
template <typename Key, typename Value>
std::vector<Key> GetKeys(const std::map<Key, Value>& map)
{
  std::vector<Key> result(map.size());
  std::transform(map.begin(), map.end(), result.begin(),
    [](const std::pair<Key, Value>& pair) { return pair.first; });
  return result;
}
//----------------------------------------------------------------------------
// Get the intersection of two maps.
template <typename Key, typename Value>
std::map<Key, Value> GetIntersection(
  const std::map<Key, Value>& map1, const std::map<Key, Value>& map2)
{
  std::map<Key, Value> intersection;
  std::set_intersection(map1.begin(), map1.end(), map2.begin(), map2.end(),
    std::inserter(intersection, intersection.begin()));

  return intersection;
}
} // namespace

//----------------------------------------------------------------------------
void vtkSMChartSeriesSelectionDomain::Update(vtkSMProperty*)
{
  vtkSMProperty* input = this->GetRequiredProperty("Input");
  if (!input)
  {
    vtkErrorMacro("Missing require property 'Input'. Update failed.");
    return;
  }
  vtkPVDataInformation* dataInfo = this->GetInputDataInformation("Input");
  if (!dataInfo)
  {
    return;
  }
  const auto fieldDataSelection =
    vtkSMIntVectorProperty::SafeDownCast(this->GetRequiredProperty("FieldDataSelection"));
  if (!fieldDataSelection)
  {
    vtkErrorMacro("Missing required property 'FieldDataSelection'. Update failed.");
    return;
  }
  const auto activeAssembly =
    vtkSMStringVectorProperty::SafeDownCast(this->GetRequiredProperty("ActiveAssembly"));
  const auto selectors =
    vtkSMStringVectorProperty::SafeDownCast(this->GetRequiredProperty("Selectors"));
  const auto arraySelectionMode =
    vtkSMIntVectorProperty::SafeDownCast(this->GetRequiredProperty("ArraySelectionMode"));

  // clear old component names.
  this->Internals->VisibilityOverrides.clear();

  if (!activeAssembly || !selectors || !dataInfo->IsCompositeDataSet())
  {
    // since there's no way to choose which dataset from a composite one to use,
    // just look at the top-level array information (skipping partial arrays).
    const int fieldAssociation = fieldDataSelection->GetUncheckedElement(0);
    auto columnNameOverrides =
      this->CollectAvailableArrays("", dataInfo, fieldAssociation, this->FlattenTable);
    this->SetDefaultVisibilityOverrides(columnNameOverrides, false);
    this->SetStrings(GetKeys(columnNameOverrides));
    return;
  }

  assert(activeAssembly->GetNumberOfUncheckedElements() == 1);

  auto assembly = dataInfo->GetDataAssembly(activeAssembly->GetUncheckedElement(0));
  const bool intersectColumnNames =
    arraySelectionMode && arraySelectionMode->GetUncheckedElement(0) == 0 /*MERGED_BLOCKS*/;
  std::vector<std::string> columnNames;
  std::map<std::string, bool> intersectedColumnNameOverrides;
  const int fieldAssociation = fieldDataSelection->GetUncheckedElement(0);
  const unsigned int numElems = selectors->GetNumberOfUncheckedElements();
  for (unsigned int cc = 0; cc < numElems; cc++)
  {
    std::string blockName;
    if (selectors->GetRepeatCommand() && !intersectColumnNames)
    {
      const auto blockNames = dataInfo->GetBlockNames(
        { selectors->GetUncheckedElement(cc) }, activeAssembly->GetUncheckedElement(0));
      blockName = blockNames.empty() ? selectors->GetUncheckedElement(cc) : blockNames.front();
    }
    const bool skipPartialArrays = intersectColumnNames && assembly &&
      assembly->GetFirstNodeByPath(selectors->GetUncheckedElement(cc)) == assembly->GetRootNode();
    auto childInfo = this->GetInputSubsetDataInformation(
      selectors->GetUncheckedElement(cc), activeAssembly->GetUncheckedElement(0), "Input");
    auto blockColumnNameOverrides = this->CollectAvailableArrays(
      blockName, childInfo, fieldAssociation, this->FlattenTable, skipPartialArrays);
    if (!intersectColumnNames)
    {
      this->SetDefaultVisibilityOverrides(blockColumnNameOverrides, false);
      auto block_column_names = GetKeys(blockColumnNameOverrides);
      columnNames.insert(columnNames.end(), block_column_names.begin(), block_column_names.end());
    }
    else if (!blockColumnNameOverrides.empty())
    {
      if (intersectedColumnNameOverrides.empty())
      {
        // if this is the first selector, we simply copy all names.
        intersectedColumnNameOverrides = blockColumnNameOverrides;
      }
      else
      {
        intersectedColumnNameOverrides =
          GetIntersection(intersectedColumnNameOverrides, blockColumnNameOverrides);
      }
    }
  }
  if (intersectColumnNames)
  {
    this->SetDefaultVisibilityOverrides(intersectedColumnNameOverrides, false);
    columnNames = GetKeys(intersectedColumnNameOverrides);
  }
  this->SetStrings(columnNames);
}

//----------------------------------------------------------------------------
// Add arrays from dataInfo to strings. If blockName is non-empty, then it's
// used to "uniquify" the array names.
std::map<std::string, bool> vtkSMChartSeriesSelectionDomain::CollectAvailableArrays(
  const std::string& blockName, vtkPVDataInformation* dataInfo, int fieldAssociation,
  bool flattenTable, bool skipPartialArrays)
{
  vtkChartRepresentation* chartRepr =
    vtkChartRepresentation::SafeDownCast(this->GetProperty()->GetParent()->GetClientSideObject());
  if (!chartRepr || !dataInfo)
  {
    return {};
  }

  // helps use avoid duplicates. duplicates may arise for plot types that treat
  // multiple columns as a single series/plot e.g. quartile plots.
  std::map<std::string, bool> stringOverrides;
  vtkPVDataSetAttributesInformation* dsa = dataInfo->GetAttributeInformation(fieldAssociation);
  if (dsa != nullptr)
  {
    std::unique_ptr<vtkPVDataSetAttributesInformation::AlphabeticalArrayInformationIterator> iter(
      dsa->NewAlphabeticalArrayInformationIterator());
    for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkPVArrayInformation* arrayInfo = iter->GetCurrentArrayInformation();
      this->CollectArrayComponents(
        chartRepr, blockName, stringOverrides, arrayInfo, flattenTable, skipPartialArrays);
    }
  }

  if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    vtkPVArrayInformation* pointArrayInfo = dataInfo->GetPointArrayInformation();
    this->CollectArrayComponents(
      chartRepr, blockName, stringOverrides, pointArrayInfo, flattenTable);
  }
  return stringOverrides;
}

//----------------------------------------------------------------------------
// Add array component from arrayInfo to strings. If blockName is non-empty, then it's
// used to "uniquify" the array names.
void vtkSMChartSeriesSelectionDomain::CollectArrayComponents(vtkChartRepresentation* chartRepr,
  const std::string& blockName, std::map<std::string, bool>& stringOverrides,
  vtkPVArrayInformation* arrayInfo, bool flattenTable, bool skipPartialArrays)
{
  auto showPartialArrays = !this->HidePartialArrays && !skipPartialArrays;
  if (arrayInfo && (showPartialArrays || arrayInfo->GetIsPartial() == 0))
  {
    int dataType = arrayInfo->GetDataType();
    if (dataType != VTK_STRING && dataType != VTK_VARIANT)
    {
      if (arrayInfo->GetNumberOfComponents() > 1 && flattenTable)
      {
        for (int kk = 0; kk <= arrayInfo->GetNumberOfComponents(); kk++)
        {
          std::string component_name = vtkSMArrayListDomain::CreateMangledName(arrayInfo, kk);
          component_name = chartRepr->GetDefaultSeriesLabel(blockName, component_name);
          if (stringOverrides.find(component_name) == stringOverrides.end())
          {
            // save component names so we can detect them when setting defaults later.
            stringOverrides.emplace(component_name, kk != arrayInfo->GetNumberOfComponents());
          }
        }
      }
      else
      {
        const char* arrayName = arrayInfo->GetName();
        if (arrayName != nullptr)
        {
          std::string seriesName = chartRepr->GetDefaultSeriesLabel(blockName, arrayName);
          if (stringOverrides.find(seriesName) == stringOverrides.end())
          {
            // Special case for Quartile plots. PlotSelectionOverTime filter, when
            // produces stats, likes to pre-split components in an array into multiple
            // single component arrays. We still want those to be treated as
            // components and not be shown by default. Hence, we use this hack.
            // (See BUG #15512).
            std::string seriesNameWithoutTableName =
              chartRepr->GetDefaultSeriesLabel("", arrayName);
            stringOverrides.emplace(
              seriesName, PotentialComponentNameRe.find(seriesNameWithoutTableName));
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkSMChartSeriesSelectionDomain::GetDefaultValue(const char* series)
{
  std::vector<std::string> values;
  if (this->DefaultMode == VISIBILITY)
  {
    values.emplace_back(this->GetDefaultSeriesVisibility(series) ? "1" : "0");
  }
  else if (this->DefaultMode == LABEL)
  {
    // by default, label is same as the name of the series.
    values.emplace_back(series);
  }
  else if (this->DefaultMode == COLOR)
  {
    double rgb[3];
    auto presetProp = this->GetProperty()->GetParent()->GetProperty("LastPresetName");
    const char* preset = presetProp ? vtkSMPropertyHelper(presetProp).GetAsString() : "Spectrum";
    this->Internals->GetNextColor(preset, rgb);
    for (int kk = 0; kk < 3; kk++)
    {
      std::ostringstream stream;
      stream << std::setprecision(2) << rgb[kk];
      values.push_back(stream.str());
    }
  }
  else if (this->DefaultMode == VALUE)
  {
    values.emplace_back(this->DefaultValue);
  }
  return values;
}

//----------------------------------------------------------------------------
int vtkSMChartSeriesSelectionDomain::SetDefaultValues(
  vtkSMProperty* property, bool use_unchecked_values)
{
  vtkSMStringVectorProperty* vp = vtkSMStringVectorProperty::SafeDownCast(property);
  if (!vp)
  {
    vtkErrorMacro("Property must be a vtkSMVectorProperty subclass.");
    return 0;
  }
  this->UpdateDefaultValues(property, false, use_unchecked_values);
  return 1;
}

//----------------------------------------------------------------------------
void vtkSMChartSeriesSelectionDomain::UpdateDefaultValues(
  vtkSMProperty* property, bool preserve_previous_values, bool use_unchecked_values)
{
  vtkSMStringVectorProperty* vp = vtkSMStringVectorProperty::SafeDownCast(property);
  assert(vp != nullptr);

  vtkNew<vtkStringList> values;
  std::set<std::string> seriesNames;
  if (preserve_previous_values)
  {
    // capture old values.
    unsigned int numElems = vp->GetNumberOfElements();
    int stepSize =
      vp->GetNumberOfElementsPerCommand() > 0 ? vp->GetNumberOfElementsPerCommand() : 1;

    for (unsigned int cc = 0; (cc + stepSize) <= numElems; cc += stepSize)
    {
      seriesNames.insert(vp->GetElement(cc) ? vp->GetElement(cc) : "");
      for (int kk = 0; kk < stepSize; kk++)
      {
        values->AddString(vp->GetElement(cc + kk));
      }
    }
  }

  const std::vector<std::string>& domain_strings = this->GetStrings();
  for (size_t cc = 0; cc < domain_strings.size(); cc++)
  {
    if (preserve_previous_values && seriesNames.find(domain_strings[cc]) != seriesNames.end())
    {
      // skip this. This series had a value set already which we are requested to preserve.
      continue;
    }
    std::vector<std::string> cur_values = this->GetDefaultValue(domain_strings[cc].c_str());
    if (!cur_values.empty())
    {
      values->AddString(domain_strings[cc].c_str());
      for (size_t kk = 0; kk < cur_values.size(); kk++)
      {
        values->AddString(cur_values[kk].c_str());
      }
    }
  }

  if (use_unchecked_values)
  {
    vp->SetUncheckedElements(values.GetPointer());
  }
  else
  {
    vp->SetElements(values.GetPointer());
  }
}

//----------------------------------------------------------------------------
bool vtkSMChartSeriesSelectionDomain::GetDefaultSeriesVisibility(const char* name)
{
  if (vtkPVGeneralSettings::GetInstance()->GetLoadNoChartVariables())
  {
    return false;
  }
  if (!vtkPVRepresentedArrayListSettings::GetInstance()->GetSeriesVisibilityDefault(name))
  {
    return false;
  }
  if (this->Internals->VisibilityOverrides.find(name) != this->Internals->VisibilityOverrides.end())
  {
    //  hide components by default, we'll show the magnitudes for them.
    return this->Internals->VisibilityOverrides[name];
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkSMChartSeriesSelectionDomain::OnDomainModified()
{
  vtkSMProperty* prop = this->GetProperty();
  this->UpdateDefaultValues(prop, true, false);
  if (prop->GetParent())
  {
    prop->GetParent()->UpdateProperty(prop->GetXMLName());
  }
}

//----------------------------------------------------------------------------
void vtkSMChartSeriesSelectionDomain::SetDefaultVisibilityOverrides(
  const std::map<std::string, bool>& arrayNames, bool visibility)
{
  for (const auto& pair : arrayNames)
  {
    if (pair.second) // should be overridden
    {
      this->Internals->VisibilityOverrides[pair.first] = visibility;
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMChartSeriesSelectionDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "HidePartialArrays: " << this->HidePartialArrays << endl;
}
