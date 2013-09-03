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
#include "vtkColorSeries.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkStdString.h"
#include "vtkStringList.h"

#include <algorithm>
#include <assert.h>
#include <map>
#include <set>
#include <vector>
#include <vtksys/ios/sstream>
#include <vtksys/RegularExpression.hxx>

vtkStandardNewMacro(vtkSMChartSeriesSelectionDomain);
namespace
{
  typedef std::pair<vtksys::RegularExpression, bool> SeriesVisibilityPair;
  static std::vector<SeriesVisibilityPair> SeriesVisibilityDefaults;
  static void InitSeriesVisibilityDefaults()
    {
    // initialize SeriesVisibilityDefaults the first time.
    if (SeriesVisibilityDefaults.size() == 0)
      {
      const char* defaults[] = {
        "^arc_length",
        "^bin_extents",
        "^FileId",
        "^GlobalElementId",
        "^GlobalNodeId",
        "^ObjectId",
        "^Pedigree.*",
        "^Points_.*",
        "^Time",
        "^vtkOriginal.*",
        "^vtkValidPointMask",
        NULL
      };
      for (int cc=0; defaults[cc] != NULL; cc++)
        {
        SeriesVisibilityDefaults.push_back(SeriesVisibilityPair(
            vtksys::RegularExpression(defaults[cc]), false));
        }
      }
    }
  static void AddSeriesVisibilityDefault(const char* regex, bool value)
    {
    InitSeriesVisibilityDefaults();
    if (regex && regex[0])
      {
      SeriesVisibilityDefaults.push_back(SeriesVisibilityPair(
          vtksys::RegularExpression(regex), value));
      }
    }

  static bool GetSeriesVisibilityDefault(const char* regex, bool &value)
    {
    InitSeriesVisibilityDefaults();
    for (size_t cc=0; cc < SeriesVisibilityDefaults.size(); cc++)
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
public:
  std::set<std::string> ComponentNames;

  static vtkColor3ub GetNextColor()
    {
    static vtkNew<vtkColorSeries> Colors;
    static int counter = 0;
    return Colors->GetColorRepeating(counter++);
    }

  // Add arrays from dataInfo to strings. If blockName is non-empty, then it's
  // used to "uniquify" the array names.
  void PopulateAvailableArrays(const std::string& blockName,
    std::vector<vtkStdString>& strings,
    vtkPVDataInformation* dataInfo, int fieldAssociation)
    {
    // this method is typically called for leaf nodes (or multi-piece).
    // assert((dataInfo->GetCompositeDataInformation()->GetDataIsComposite() == 0) ||
    //   (dataInfo->GetCompositeDataInformation()->GetDataIsMultiPiece() == 0));

    vtkPVDataSetAttributesInformation* dsa =
      dataInfo->GetAttributeInformation(fieldAssociation);
    for (int cc=0; dsa != NULL && cc < dsa->GetNumberOfArrays(); cc++)
      {
      vtkPVArrayInformation* arrayInfo = dsa->GetArrayInformation(cc);
      if (arrayInfo == NULL || arrayInfo->GetIsPartial() == 1)
        {
        continue;
        }

      if (arrayInfo->GetName() &&
        strcmp(arrayInfo->GetName(), "vtkValidPointMask") == 0)
        {
        // Skip vtkValidPointMask since that's an internal array used for masking,
        // not really meant to be plotted.
        continue;
        }

      if (arrayInfo->GetNumberOfComponents() > 1)
        {
        for (int kk=0; kk <= arrayInfo->GetNumberOfComponents(); kk++)
          {
          std::string component_name = vtkSMArrayListDomain::CreateMangledName(arrayInfo, kk);
          component_name = vtkChartRepresentation::GetDefaultSeriesLabel(
            blockName, component_name);

          strings.push_back(component_name);
          if (kk != arrayInfo->GetNumberOfComponents())
            {
            // save component names so we can detect them when setting defaults
            // later.
            this->ComponentNames.insert(component_name);
            }
          }
        }
      else
        {
        strings.push_back(
          vtkChartRepresentation::GetDefaultSeriesLabel(blockName, arrayInfo->GetName()));
        }
      }
    }
};

//----------------------------------------------------------------------------
vtkSMChartSeriesSelectionDomain::vtkSMChartSeriesSelectionDomain() :
  Internals(new vtkSMChartSeriesSelectionDomain::vtkInternals())
{
  this->DefaultMode = vtkSMChartSeriesSelectionDomain::UNDEFINED;
  this->DefaultValue = 0;
  this->SetDefaultValue("");
}

//----------------------------------------------------------------------------
vtkSMChartSeriesSelectionDomain::~vtkSMChartSeriesSelectionDomain()
{
  this->SetDefaultValue(NULL);
  delete this->Internals;
  this->Internals = NULL;
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
  return NULL;
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

  return 1;
}

//----------------------------------------------------------------------------
void vtkSMChartSeriesSelectionDomain::Update(vtkSMProperty*)
{
  vtkSMProperty* input = this->GetRequiredProperty("Input");
  vtkSMProperty* fieldDataSelection =
    this->GetRequiredProperty("FieldDataSelection");
  vtkSMVectorProperty* compositeIndex = vtkSMVectorProperty::SafeDownCast(
    this->GetRequiredProperty("CompositeIndexSelection"));

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
  this->Internals->ComponentNames.clear();


  if (compositeIndex == NULL ||
    dataInfo->GetCompositeDataInformation()->GetDataIsComposite() == 0)
    {
    // since there's no way to choose which dataset from a composite one to use,
    // just look at the top-level array information (skipping partial arrays).
    std::vector<vtkStdString> column_names;
    int fieldAssociation = vtkSMUncheckedPropertyHelper(fieldDataSelection).GetAsInt(0);
    this->Internals->PopulateAvailableArrays(std::string(),
      column_names, dataInfo, fieldAssociation);
    this->SetStrings(column_names);
    return;
    }

  std::vector<vtkStdString> column_names;
  int fieldAssociation = vtkSMUncheckedPropertyHelper(fieldDataSelection).GetAsInt(0);

  vtkSMUncheckedPropertyHelper compositeIndexHelper(compositeIndex);
  unsigned int numElems = compositeIndexHelper.GetNumberOfElements();
  for (unsigned int cc=0; cc < numElems; cc++)
    {
    vtkPVDataInformation* childInfo =
      dataInfo->GetDataInformationForCompositeIndex(compositeIndexHelper.GetAsInt(cc));
    if (!childInfo)
      {
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
    this->Internals->PopulateAvailableArrays(blockNameStream.str(),
      column_names, dataInfo, fieldAssociation);
    }
  this->SetStrings(column_names);
}

//----------------------------------------------------------------------------
int vtkSMChartSeriesSelectionDomain::SetDefaultValues(vtkSMProperty* property)
{
  vtkSMStringVectorProperty* vp = vtkSMStringVectorProperty::SafeDownCast(property);
  if (!vp)
    {
    vtkErrorMacro("Property must be a vtkSMVectorProperty subclass.");
    return 0;
    }

  const std::vector<vtkStdString>& domain_strings = this->GetStrings();
  if (this->DefaultMode == VISIBILITY)
    {
    vtkNew<vtkStringList> value;
    for (size_t cc=0; cc < domain_strings.size(); cc++)
      {
      value->AddString(domain_strings[cc].c_str());
      value->AddString(
        this->GetDefaultSeriesVisibility(domain_strings[cc].c_str())?
        "1" : "0");
      }
    vp->SetElements(value.GetPointer());
    }
  else if (this->DefaultMode == LABEL)
    {
    vtkNew<vtkStringList> value;
    for (size_t cc=0; cc < domain_strings.size(); cc++)
      {
      // by default, label is same as the name of the series.
      value->AddString(domain_strings[cc].c_str());
      value->AddString(domain_strings[cc].c_str());
      }
    vp->SetElements(value.GetPointer());
    }
  else if (this->DefaultMode == COLOR)
    {
    vtkNew<vtkStringList> value;
    for (size_t cc=0; cc < domain_strings.size(); cc++)
      {
      // by default, label is same as the name of the series.
      value->AddString(domain_strings[cc].c_str());
      vtkColor3ub color = this->Internals->GetNextColor();
      for (int kk=0; kk < 3; kk++)
        {
        std::ostringstream stream;
        stream << std::setprecision(2) << color.GetData()[kk]/255.0;
        value->AddString(stream.str().c_str());
        }
      }
    vp->SetElements(value.GetPointer());
    }
  else if (this->DefaultMode == VALUE)
    {
    assert(vp->GetNumberOfElementsPerCommand() == 2);
    vtkNew<vtkStringList> value;
    for (size_t cc=0; cc < domain_strings.size(); cc++)
      {
      // by default, label is same as the name of the series.
      value->AddString(domain_strings[cc].c_str());
      value->AddString(this->DefaultValue);
      }
    vp->SetElements(value.GetPointer());
    }

  return 0;
}


//----------------------------------------------------------------------------
void vtkSMChartSeriesSelectionDomain::AddSeriesVisibilityDefault(
  const char* name, bool value)
{
  AddSeriesVisibilityDefault(name, value);
}

//----------------------------------------------------------------------------
bool vtkSMChartSeriesSelectionDomain::GetDefaultSeriesVisibility(const char* name)
{
  bool result;
  if (GetSeriesVisibilityDefault(name, result))
    {
    return result;
    }

  if (this->Internals->ComponentNames.find(name) !=
    this->Internals->ComponentNames.end())
    {
    //  hide components by default, we'll show the magnitudes for them.
    return false;
    }

  return true;
}


//----------------------------------------------------------------------------
void vtkSMChartSeriesSelectionDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
