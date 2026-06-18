// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVPostFilterPrivateTools.h"

#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkStringScanner.h"

#if VTK_MODULE_ENABLE_VTK_FiltersCore
#include "vtkCellDataToPointData.h"
#include "vtkPointDataToCellData.h"
#endif

#include <cassert>
#include <sstream>
#include <vector>

//----------------------------------------------------------------------------
std::string vtkPVPostFilterPrivateTools::DefaultComponentName(
  int componentNumber, int componentCount)
{
  if (componentCount <= 1)
  {
    return "";
  }
  else if (componentNumber == -1)
  {
    return "Magnitude";
  }
  else if (componentCount <= 3 && componentNumber < 3)
  {
    const char* titles[] = { "X", "Y", "Z" };
    return titles[componentNumber];
  }
  else if (componentCount == 6)
  {
    const char* titles[] = { "XX", "YY", "ZZ", "XY", "YZ", "XZ" };
    // Assume this is a symmetric matrix.
    return titles[componentNumber];
  }
  else
  {
    std::ostringstream buffer;
    buffer << componentNumber;
    return buffer.str();
  }
}

//----------------------------------------------------------------------------
void vtkPVPostFilterPrivateTools::DeMangleArrayName(const std::string& mangledName,
  vtkDataSet* dataSet, std::string& demangledName, std::string& demangledComponentName)
{
  std::vector<vtkDataSetAttributes*> attributesArray;
  attributesArray.push_back(dataSet->GetCellData());
  attributesArray.push_back(dataSet->GetPointData());

  for (size_t index = 0; index < attributesArray.size(); index++)
  {
    vtkDataSetAttributes* dataSetAttributes = attributesArray[index];
    if (!dataSetAttributes)
    {
      continue;
    }

    for (int arrayIndex = 0; arrayIndex < dataSetAttributes->GetNumberOfArrays(); arrayIndex++)
    {
      // check for matching array name at the start of the mangled name
      const char* arrayName = dataSetAttributes->GetArrayName(arrayIndex);
      size_t arrayNameLength = strlen(arrayName);
      if (strncmp(mangledName.c_str(), arrayName, arrayNameLength) == 0)
      {
        if (mangledName.size() == arrayNameLength)
        {
          // the mangled name is just the array name
          demangledName = mangledName;
          demangledComponentName = std::string();
          return;
        }
        else if (mangledName.size() > arrayNameLength + 1)
        {
          vtkAbstractArray* array = dataSetAttributes->GetAbstractArray(arrayIndex);
          int componentCount = array->GetNumberOfComponents();

          // check the for a matching component name
          // has to be from -1 as -1 represents the Magnitude component
          for (int componentIndex = -1; componentIndex < componentCount; componentIndex++)
          {
            std::string componentNameString;
            const char* componentName = array->GetComponentName(componentIndex);
            if (componentName)
            {
              componentNameString = componentName;
            }
            else
            {
              // use the default component name if the component has no name set
              componentNameString =
                vtkPVPostFilterPrivateTools::DefaultComponentName(componentIndex, componentCount);
            }

            if (componentNameString.empty())
            {
              continue;
            }

            // check component name from the end of array name string after the underscore
            const char* mangledComponentName = &mangledName[arrayNameLength + 1];
            if (componentNameString == mangledComponentName)
            {
              // found a match
              demangledName = arrayName;
              demangledComponentName = mangledComponentName;
              return;
            }
          }
        }
      }
    }
  }

  // return original name
  demangledName = mangledName;
  demangledComponentName = std::string();
}

//----------------------------------------------------------------------------
bool vtkPVPostFilterPrivateTools::ExtractComponent(vtkDataSetAttributes* dsa,
  const char* requested_name, const char* demangled_name, const char* demangled_component_name)
{
  vtkAbstractArray* array = dsa->GetAbstractArray(demangled_name);
  assert(array != nullptr && demangled_name && demangled_component_name);

  int cIndex = -1;
  bool found = false;
  // demangled_component_name can be a real component name OR
  // X,Y,Z for the first 3 components OR
  // 0,...N i.e. an integer for the index OR
  // Magnitude to indicate vector magnitude.
  // Now to the trick is to decide what way this particular request has been
  // made.

  // First pass is to match the demangled name to a component name
  // Component names take highest priority so if somebody named component 4 to be "1"
  // that should match before resorting to using atoi
  for (int cc = 0; cc < array->GetNumberOfComponents() && array->HasAComponentName(); cc++)
  {
    const char* comp_name = array->GetComponentName(cc);
    if (comp_name && strcmp(comp_name, demangled_component_name) == 0)
    {
      cIndex = cc;
      found = true;
      break;
    }
  }

  // if we still haven't found a match we will check the component against the
  // the default names.
  int numComps = array->GetNumberOfComponents();
  // compare against cIndex to only run this if component names didn't match
  for (int i = -1; i < numComps && cIndex == -1; i++)
  {
    std::string defaultName = DefaultComponentName(i, numComps);
    if (vtksys::SystemTools::Strucmp(defaultName.c_str(), demangled_component_name) == 0)
    {
      cIndex = i;
      found = true;
      break;
    }
  }

  // None of the component names or default names matched so we
  // go onto doing a pure conversion of the string to integer and using that.
  if (!found)
  {
    VTK_FROM_CHARS_IF_ERROR_RETURN(demangled_component_name, cIndex, false);
  }

  // when we compute the magnitude we must place
  // the result in a double array, since we don't the size of the
  // resulting data.
  bool isMagnitude = (cIndex == -1);

  vtkSmartPointer<vtkAbstractArray> newArray;
  if (isMagnitude)
  {
    newArray = vtk::ComponentOrNormAsArray(array, vtkArrayComponents::L2Norm);
  }
  else
  {
    newArray = vtk::ComponentOrNormAsArray(array, std::max(cIndex, 0));
  }
  newArray->SetName(requested_name);
  dsa->AddArray(newArray);
  return true;
}

//----------------------------------------------------------------------------
void vtkPVPostFilterPrivateTools::CellDataToPointData(vtkDataSet* output, const char* name)
{
  vtkDataObject* clone = output->NewInstance();
  clone->ShallowCopy(output);

#if VTK_MODULE_ENABLE_VTK_FiltersCore
  vtkCellDataToPointData* converter = vtkCellDataToPointData::New();
  converter->SetInputData(clone);
  converter->PassCellDataOn();
  converter->ProcessAllArraysOff();
  converter->AddCellDataArray(name);
  converter->Update();
  output->ShallowCopy(converter->GetOutputDataObject(0));
  converter->Delete();
  clone->Delete();
#else
  vtkWarningMacro(
    "`vtkCellDataToPointData` is not available in your build. Please enable appropriate module.");
#endif
}

//----------------------------------------------------------------------------
void vtkPVPostFilterPrivateTools::PointDataToCellData(vtkDataSet* output, const char* name)
{
  vtkDataObject* clone = output->NewInstance();
  clone->ShallowCopy(output);

#if VTK_MODULE_ENABLE_VTK_FiltersCore
  vtkPointDataToCellData* converter = vtkPointDataToCellData::New();
  converter->SetInputData(clone);
  converter->PassPointDataOn();
  converter->ProcessAllArraysOff();
  converter->AddPointDataArray(name);
  converter->Update();
  output->ShallowCopy(converter->GetOutputDataObject(0));
  converter->Delete();
  clone->Delete();
#else
  vtkWarningMacro(
    "`vtkPointDataToCellData` is not available in your build. Please enable appropriate module.");
#endif
}

//----------------------------------------------------------------------------
bool vtkPVPostFilterPrivateTools::DoAnyNeededConversions(vtkDataSet* output,
  const char* requested_name, int fieldAssociation, const char* demangled_name,
  const char* demangled_component_name, bool checkOnly)
{
  vtkDataSetAttributes* dsa = nullptr;
  vtkDataSetAttributes* pointData = output->GetPointData();
  vtkDataSetAttributes* cellData = output->GetCellData();

  switch (fieldAssociation)
  {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
      dsa = pointData;
      break;

    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      dsa = cellData;
      break;

    case vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS:
      vtkWarningWithObjectMacro(nullptr, "Case not handled");
      [[fallthrough]];

    default:
      return false;
  }

  if (dsa->GetAbstractArray(requested_name))
  {
    // requested array is present. Don't bother doing anything.
    return false;
  }

  if (checkOnly)
  {
    return true;
  }

  if (dsa->GetAbstractArray(demangled_name))
  {
    // demangled_name is present, implies that component extraction is needed.
    vtkPVPostFilterPrivateTools::ExtractComponent(
      dsa, requested_name, demangled_name, demangled_component_name);
    return true;
  }

  if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    vtkAbstractArray* array = cellData->GetAbstractArray(requested_name);
    if (!array)
    {
      array = cellData->GetAbstractArray(demangled_name);
    }
    if (array)
    {
      vtkPVPostFilterPrivateTools::CellDataToPointData(output, array->GetName());
    }
  }
  else if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    vtkAbstractArray* array = pointData->GetAbstractArray(requested_name);
    if (!array)
    {
      array = pointData->GetAbstractArray(demangled_name);
    }
    if (array)
    {
      vtkPVPostFilterPrivateTools::PointDataToCellData(output, array->GetName());
    }
  }

  if (dsa->GetAbstractArray(requested_name))
  {
    // after the conversion the requested array is present. Don't bother doing
    // anything more.
    return true;
  }

  if (dsa->GetAbstractArray(demangled_name))
  {
    // demangled_name is present, implies that component extraction is needed.
    vtkPVPostFilterPrivateTools::ExtractComponent(
      dsa, requested_name, demangled_name, demangled_component_name);
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkPVPostFilterPrivateTools::DoAnyNeededConversions(vtkInformation* info,
  vtkInformationInformationVectorKey* key, vtkDataSet* dataset, bool checkOnly)
{
  bool convert = false;
  // get the array to convert info
  vtkInformationVector* postVector = info->Get(key);
  for (int i = 0; i < postVector->GetNumberOfInformationObjects(); i++)
  {
    vtkInformation* postArrayInfo = postVector->GetInformationObject(i);

    const char* name = postArrayInfo->Get(vtkDataObject::FIELD_NAME());
    int fieldAssociation = postArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION());

    std::string demangled_name, demangled_component_name;
    DeMangleArrayName(name, dataset, demangled_name, demangled_component_name);

    convert |= vtkPVPostFilterPrivateTools::DoAnyNeededConversions(dataset, name, fieldAssociation,
      demangled_name.c_str(), demangled_component_name.c_str(), checkOnly);
  }
  return convert;
}

//----------------------------------------------------------------------------
bool vtkPVPostFilterPrivateTools::DoAnyNeededConversions(vtkInformation* info,
  vtkInformationInformationVectorKey* key, vtkDataObject* output, bool checkOnly)
{
  bool convert = false;
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(output);
  if (cd)
  {
    vtkCompositeDataIterator* iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataSet* dataset = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (dataset)
      {
        convert |=
          vtkPVPostFilterPrivateTools::DoAnyNeededConversions(info, key, dataset, checkOnly);
      }
    }
    iter->Delete();
  }
  else
  {
    vtkDataSet* dataset = vtkDataSet::SafeDownCast(output);
    if (dataset)
    {
      convert = vtkPVPostFilterPrivateTools::DoAnyNeededConversions(info, key, dataset, checkOnly);
    }
  }
  return convert;
}
