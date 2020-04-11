/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMChartSeriesSelectionDomain.h"

#include "vtkChartRepresentation.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTransferFunctionPresets.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkStringList.h"

#include <vtk_jsoncpp.h>

#include <algorithm>
#include <assert.h>
#include <map>
#include <set>
#include <sstream>
#include <vector>
#include <vtksys/RegularExpression.hxx>

vtkStandardNewMacro(vtkSMChartSeriesSelectionDomain);
namespace
{
// match string like: "ACCL (0)" or "VEL (1)"
static vtksys::RegularExpression PotentailComponentNameRe(".*\\([0-9]+\\)");

typedef std::pair<vtksys::RegularExpression, bool> SeriesVisibilityPair;
static std::vector<SeriesVisibilityPair> SeriesVisibilityDefaults;
static void InitSeriesVisibilityDefaults()
{
  // initialize SeriesVisibilityDefaults the first time.
  if (SeriesVisibilityDefaults.size() == 0)
  {
    const char* defaults[] = { "^arc_length", "^bin_extents", "^FileId", "^GlobalElementId",
      "^GlobalNodeId", "^ObjectId", "^Pedigree.*", "^Points_.*", "^Time", "^vtkOriginal.*",
      "^vtkValidPointMask", "^N .*", "^X$", "^X .*", "^Y$", "^Y .*", "^Z$", "^Z .*",
      "^vtkGhostType$", nullptr };
    for (int cc = 0; defaults[cc] != nullptr; cc++)
    {
      SeriesVisibilityDefaults.push_back(
        SeriesVisibilityPair(vtksys::RegularExpression(defaults[cc]), false));
    }
  }
}
static void AddSeriesVisibilityDefault(const char* regex, bool value)
{
  InitSeriesVisibilityDefaults();
  if (regex && regex[0])
  {
    SeriesVisibilityDefaults.push_back(
      SeriesVisibilityPair(vtksys::RegularExpression(regex), value));
  }
}

static bool GetSeriesVisibilityDefault(const char* regex, bool& value)
{
  InitSeriesVisibilityDefaults();
  for (size_t cc = 0; cc < SeriesVisibilityDefaults.size(); cc++)
  {
    if (SeriesVisibilityDefaults[cc].first.find(regex))
    {
      value = SeriesVisibilityDefaults[cc].second;
      return true;
    }
  }
  return false;
}
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

//---------------------------------------------------------------------------
bool vtkSMChartSeriesSelectionDomain::LoadNoVariables = false;

//----------------------------------------------------------------------------
vtkSMChartSeriesSelectionDomain::vtkSMChartSeriesSelectionDomain()
  : Internals(new vtkSMChartSeriesSelectionDomain::vtkInternals())
{
  this->DefaultMode = vtkSMChartSeriesSelectionDomain::UNDEFINED;
  this->DefaultValue = 0;
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

//----------------------------------------------------------------------------
void vtkSMChartSeriesSelectionDomain::Update(vtkSMProperty*)
{
  vtkSMProperty* input = this->GetRequiredProperty("Input");
  vtkSMProperty* fieldDataSelection = this->GetRequiredProperty("FieldDataSelection");
  vtkSMVectorProperty* compositeIndex =
    vtkSMVectorProperty::SafeDownCast(this->GetRequiredProperty("CompositeIndexSelection"));

  if (!input || !fieldDataSelection)
  {
    vtkWarningMacro("Missing required properties. Update failed.");
    return;
  }

  // build strings based on the current domain.
  vtkPVDataInformation* dataInfo = this->GetInputInformation();
  if (!dataInfo)
  {
    return;
  }

  // clear old component names.
  this->Internals->VisibilityOverrides.clear();

  if (compositeIndex == nullptr ||
    dataInfo->GetCompositeDataInformation()->GetDataIsComposite() == 0)
  {
    // since there's no way to choose which dataset from a composite one to use,
    // just look at the top-level array information (skipping partial arrays).
    std::vector<std::string> column_names;
    int fieldAssociation = vtkSMUncheckedPropertyHelper(fieldDataSelection).GetAsInt(0);
    this->PopulateAvailableArrays(
      std::string(), column_names, dataInfo, fieldAssociation, this->FlattenTable);
    this->SetStrings(column_names);
    return;
  }

  std::vector<std::string> column_names;
  int fieldAssociation = vtkSMUncheckedPropertyHelper(fieldDataSelection).GetAsInt(0);

  vtkSMUncheckedPropertyHelper compositeIndexHelper(compositeIndex);
  unsigned int numElems = compositeIndexHelper.GetNumberOfElements();
  for (unsigned int cc = 0; cc < numElems; cc++)
  {
    vtkPVDataInformation* childInfo =
      dataInfo->GetDataInformationForCompositeIndex(compositeIndexHelper.GetAsInt(cc));
    if (!childInfo)
    {
      continue;
    }
    vtkPVCompositeDataInformation* childCompositeInfo = childInfo->GetCompositeDataInformation();
    if (childCompositeInfo->GetNumberOfChildren() != 0)
    {
      // Ignore non leaf block
      continue;
    }
    std::ostringstream blockNameStream;
    if (compositeIndex->GetRepeatCommand())
    {
      // we don't need to add blockName is the proxy doesn't support selecting
      // multiple blocks in the dataset.
      if (childInfo->GetCompositeDataSetName())
      {
        blockNameStream << childInfo->GetCompositeDataSetName();
      }
      else
      {
        blockNameStream << compositeIndexHelper.GetAsInt(cc);
      }
    }
    this->PopulateAvailableArrays(
      blockNameStream.str(), column_names, childInfo, fieldAssociation, this->FlattenTable);
  }
  this->SetStrings(column_names);
}

//----------------------------------------------------------------------------
// Add arrays from dataInfo to strings. If blockName is non-empty, then it's
// used to "uniquify" the array names.
void vtkSMChartSeriesSelectionDomain::PopulateAvailableArrays(const std::string& blockName,
  std::vector<std::string>& strings, vtkPVDataInformation* dataInfo, int fieldAssociation,
  bool flattenTable)
{
  // this method is typically called for leaf nodes (or multi-piece).
  // assert((dataInfo->GetCompositeDataInformation()->GetDataIsComposite() == 0) ||
  //   (dataInfo->GetCompositeDataInformation()->GetDataIsMultiPiece() == 0));
  vtkChartRepresentation* chartRepr =
    vtkChartRepresentation::SafeDownCast(this->GetProperty()->GetParent()->GetClientSideObject());
  if (!chartRepr)
  {
    return;
  }

  // helps use avoid duplicates. duplicates may arise for plot types that treat
  // multiple columns as a single series/plot e.g. quartile plots.
  std::set<std::string> uniquestrings;

  vtkPVDataSetAttributesInformation* dsa = dataInfo->GetAttributeInformation(fieldAssociation);
  for (int cc = 0; dsa != nullptr && cc < dsa->GetNumberOfArrays(); cc++)
  {
    vtkPVArrayInformation* arrayInfo = dsa->GetArrayInformation(cc);
    this->PopulateArrayComponents(
      chartRepr, blockName, strings, uniquestrings, arrayInfo, flattenTable);
  }

  if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    vtkPVArrayInformation* pointArrayInfo = dataInfo->GetPointArrayInformation();
    this->PopulateArrayComponents(
      chartRepr, blockName, strings, uniquestrings, pointArrayInfo, flattenTable);
  }
}

//----------------------------------------------------------------------------
// Add array component from arrayInfo to strings. If blockName is non-empty, then it's
// used to "uniquify" the array names.
void vtkSMChartSeriesSelectionDomain::PopulateArrayComponents(vtkChartRepresentation* chartRepr,
  const std::string& blockName, std::vector<std::string>& strings,
  std::set<std::string>& uniquestrings, vtkPVArrayInformation* arrayInfo, bool flattenTable)
{
  if (arrayInfo && (!this->HidePartialArrays || arrayInfo->GetIsPartial() == 0))
  {
    int dataType = arrayInfo->GetDataType();
    if (dataType != VTK_STRING && dataType != VTK_UNICODE_STRING && dataType != VTK_VARIANT)
    {
      if (arrayInfo->GetNumberOfComponents() > 1 && flattenTable)
      {
        for (int kk = 0; kk <= arrayInfo->GetNumberOfComponents(); kk++)
        {
          std::string component_name = vtkSMArrayListDomain::CreateMangledName(arrayInfo, kk);
          component_name = chartRepr->GetDefaultSeriesLabel(blockName, component_name);
          if (uniquestrings.find(component_name) == uniquestrings.end())
          {
            strings.push_back(component_name);
            uniquestrings.insert(component_name);
          }
          if (kk != arrayInfo->GetNumberOfComponents())
          {
            // save component names so we can detect them when setting defaults
            // later.
            this->SetDefaultVisibilityOverride(component_name, false);
          }
        }
      }
      else
      {
        char* arrayName = arrayInfo->GetName();
        if (arrayName != nullptr)
        {
          std::string seriesName = chartRepr->GetDefaultSeriesLabel(blockName, arrayName);
          if (uniquestrings.find(seriesName) == uniquestrings.end())
          {
            strings.push_back(seriesName);
            uniquestrings.insert(seriesName);

            // Special case for Quartile plots. PlotSelectionOverTime filter, when
            // produces stats, likes to pre-split components in an array into multiple
            // single component arrays. We still want those to be treated as
            // components and not be shown by default. Hence, we use this hack.
            // (See BUG #15512).
            std::string seriesNameWithoutTableName =
              chartRepr->GetDefaultSeriesLabel(std::string(), arrayName);
            if (PotentailComponentNameRe.find(seriesNameWithoutTableName))
            {
              this->SetDefaultVisibilityOverride(seriesName, false);
            }
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
    values.push_back(this->GetDefaultSeriesVisibility(series) ? "1" : "0");
  }
  else if (this->DefaultMode == LABEL)
  {
    // by default, label is same as the name of the series.
    values.push_back(series);
  }
  else if (this->DefaultMode == COLOR)
  {
    double rgb[3];
    auto presetProp = this->GetProperty()->GetParent()->GetProperty("LastPresetName");
    this->Internals->GetNextColor(
      presetProp ? vtkSMPropertyHelper(presetProp).GetAsString() : "Spectrum", rgb);
    for (int kk = 0; kk < 3; kk++)
    {
      std::ostringstream stream;
      stream << std::setprecision(2) << rgb[kk];
      values.push_back(stream.str());
    }
  }
  else if (this->DefaultMode == VALUE)
  {
    values.push_back(this->DefaultValue);
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
  if (use_unchecked_values)
  {
    vtkWarningMacro("Developer warning: use_unchecked_values not implemented yet.");
  }

  this->UpdateDefaultValues(property, false);
  return 1;
}

//----------------------------------------------------------------------------
void vtkSMChartSeriesSelectionDomain::UpdateDefaultValues(
  vtkSMProperty* property, bool preserve_previous_values)
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
      // skip this. This series had a value set already which we are requested
      // to preserve.
      continue;
    }
    std::vector<std::string> cur_values = this->GetDefaultValue(domain_strings[cc].c_str());
    if (cur_values.size() > 0)
    {
      values->AddString(domain_strings[cc].c_str());
      for (size_t kk = 0; kk < cur_values.size(); kk++)
      {
        values->AddString(cur_values[kk].c_str());
      }
    }
  }

  vp->SetElements(values.GetPointer());
}

//----------------------------------------------------------------------------
void vtkSMChartSeriesSelectionDomain::AddSeriesVisibilityDefault(const char* name, bool value)
{
  ::AddSeriesVisibilityDefault(name, value);
}

//----------------------------------------------------------------------------
bool vtkSMChartSeriesSelectionDomain::GetDefaultSeriesVisibility(const char* name)
{
  bool result;
  if (vtkSMChartSeriesSelectionDomain::LoadNoVariables)
  {
    return false;
  }
  if (::GetSeriesVisibilityDefault(name, result))
  {
    return result;
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
  this->UpdateDefaultValues(prop, true);
  if (prop->GetParent())
  {
    // FIXME:
    // prop->GetParent()->UpdateProperty(prop);
    prop->GetParent()->UpdateVTKObjects();
  }
}

//----------------------------------------------------------------------------
void vtkSMChartSeriesSelectionDomain::SetDefaultVisibilityOverride(
  const std::string& arrayname, bool visibility)
{
  this->Internals->VisibilityOverrides[arrayname] = visibility;
}

//----------------------------------------------------------------------------
void vtkSMChartSeriesSelectionDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "HidePartialArrays: " << this->HidePartialArrays << endl;
}
