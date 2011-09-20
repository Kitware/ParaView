/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPVPostFilter.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPostFilter.h"

#include "vtkArrayIteratorIncludes.h"
#include "vtkCellData.h"
#include "vtkCellDataToPointData.h"
#include "vtkCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkPVPostFilterExecutive.h"

#include <vtksys/SystemTools.hxx>
#include <vtkstd/string>
#include <assert.h>
#include <sstream>

namespace
{
  // Demangles a mangled string containing an array name and a component name.
  void DeMangleArrayName(const vtkstd::string &mangledName,
                         vtkDataSet *dataSet,
                         vtkstd::string &demangledName,
                         vtkstd::string &demangledComponentName)
    {
    std::vector<vtkDataSetAttributes *> attributesArray;
    attributesArray.push_back(dataSet->GetCellData());
    attributesArray.push_back(dataSet->GetPointData());

    for(size_t index = 0; index < attributesArray.size(); index++)
      {
      vtkDataSetAttributes *dataSetAttributes = attributesArray[index];
      if(!dataSetAttributes)
        {
        continue;
        }

      for(int arrayIndex = 0; arrayIndex < dataSetAttributes->GetNumberOfArrays(); arrayIndex++)
        {
        // check for matching array name at the start of the mangled name
        const char *arrayName = dataSetAttributes->GetArrayName(arrayIndex);
        size_t arrayNameLength = strlen(arrayName);
        if(strncmp(mangledName.c_str(), arrayName, arrayNameLength) == 0)
          {
          if(mangledName.size() == arrayNameLength)
            {
            // the mangled name is just the array name
            demangledName = mangledName;
            demangledComponentName = vtkstd::string();
            return;
            }
          else if(mangledName.size() > arrayNameLength + 1)
            {
            vtkAbstractArray *array = dataSetAttributes->GetAbstractArray(arrayIndex);
            size_t componentCount = array->GetNumberOfComponents();

            // check the for a matching component name
            for(size_t componentIndex = 0; componentIndex < componentCount; componentIndex++)
              {
              vtkStdString componentNameString;
              const char *componentName = array->GetComponentName(componentIndex);
              if(componentName)
                {
                componentNameString = componentName;
                }
              else
                {
                // use the default component name if the component has no name set
                componentNameString = vtkPVPostFilter::DefaultComponentName(componentIndex, componentCount);
                }

              if(componentNameString.empty())
                {
                continue;
                }

              // check component name from the end of array name string after the underscore
              const char *mangledComponentName = &mangledName[arrayNameLength+1];
              if(componentNameString == mangledComponentName)
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
    demangledComponentName = vtkstd::string();
    }
}

vtkStandardNewMacro(vtkPVPostFilter);
//----------------------------------------------------------------------------
vtkPVPostFilter::vtkPVPostFilter()
{
  vtkPVPostFilterExecutive* exec = vtkPVPostFilterExecutive::New();
  this->SetExecutive(exec);
  exec->FastDelete();

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->GetInformation()->Set(vtkAlgorithm::PRESERVES_DATASET(), 1);
}

//----------------------------------------------------------------------------
vtkPVPostFilter::~vtkPVPostFilter()
{
}

//----------------------------------------------------------------------------
vtkExecutive* vtkPVPostFilter::CreateDefaultExecutive()
{
  return vtkPVPostFilterExecutive::New();
}

//----------------------------------------------------------------------------
vtkStdString vtkPVPostFilter::DefaultComponentName(int componentNumber, int componentCount)
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
    const char* titles[] = {"X", "Y", "Z"};
    return titles[componentNumber];
    }
  else if (componentCount == 6)
    {
    const char* titles[] = {"XX", "YY", "ZZ", "XY", "YZ", "XZ"};
    // Assume this is a symmetric matrix.
    return titles[componentNumber];
    }
  else
    {
    vtkstd::ostringstream buffer;
    buffer << componentNumber;
    return buffer.str();
    }
}

//----------------------------------------------------------------------------
int vtkPVPostFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // We want to exclude vtkTemporalDataSet from being accepted as an input,
  // everything else is acceptable.
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGenericDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHierarchicalBoxDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVPostFilter::RequestDataObject(
  vtkInformation*,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (input)
    {
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());
      if (!output || !output->IsA(input->GetClassName()))
        {
        vtkDataObject* newOutput = input->NewInstance();
        newOutput->SetPipelineInformation(info);
        newOutput->Delete();
        }
      }
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVPostFilter::RequestData(vtkInformation *,
  vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{

  //we need to just copy the data, so we can fixup the output as needed
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* input= inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output= outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (output && input)
      {
      vtkCompositeDataSet *csInput = vtkCompositeDataSet::SafeDownCast(input);
      vtkCompositeDataSet *csOutput = vtkCompositeDataSet::SafeDownCast(output);
      if (!csInput && !csOutput)
        {
        //vtkDataSet
        output->ShallowCopy(input);
        }
      else
        {
        csOutput->CopyStructure(csInput);
        vtkCompositeDataIterator* iter = csInput->NewIterator();
        for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
          {
          vtkDataObject* obj = iter->GetCurrentDataObject()->NewInstance();
          obj->ShallowCopy(iter->GetCurrentDataObject());
          csOutput->SetDataSet(iter,obj);
          obj->FastDelete();
          }
        iter->Delete();
        }
      if (this->Information->Has(vtkPVPostFilterExecutive::POST_ARRAYS_TO_PROCESS()) )
        {
        this->DoAnyNeededConversions(output);
        }
      }
    return 1;
}

//----------------------------------------------------------------------------
int vtkPVPostFilter::DoAnyNeededConversions(vtkDataObject* output)
{
  //get the array to convert info
  vtkInformationVector* postVector =
    this->Information->Get(vtkPVPostFilterExecutive::POST_ARRAYS_TO_PROCESS());
  vtkInformation *postArrayInfo = postVector->GetInformationObject(0);

  const char* name = postArrayInfo->Get(vtkDataObject::FIELD_NAME());
  int fieldAssociation = postArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION());

  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(output);
  if (cd)
    {
    vtkCompositeDataIterator* iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
      iter->GoToNextItem())
      {
      vtkDataSet* dataset = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (dataset)
        {
        vtkstd::string demangled_name, demagled_component_name;
        DeMangleArrayName(name, dataset, demangled_name, demagled_component_name);

        this->DoAnyNeededConversions(dataset, name, fieldAssociation,
          demangled_name.c_str(), demagled_component_name.c_str());
        }
      }
    iter->Delete();
    return 1;
    }
  else
    {
    vtkDataSet* dataset = vtkDataSet::SafeDownCast(output);
    if (dataset)
      {
      vtkstd::string demangled_name, demagled_component_name;
      DeMangleArrayName(name, dataset, demangled_name, demagled_component_name);

      return this->DoAnyNeededConversions(dataset,
                                          name,
                                          fieldAssociation,
                                          demangled_name.c_str(),
                                          demagled_component_name.c_str());
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVPostFilter::DoAnyNeededConversions(vtkDataSet* output,
  const char* requested_name, int fieldAssociation,
  const char* demangled_name, const char* demagled_component_name)
{
  vtkDataSetAttributes* dsa = NULL;
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
    vtkWarningMacro("Case not handled");

  default:
    return 0;
    }

  if (dsa->GetAbstractArray(requested_name))
    {
    // requested array is present. Don't bother doing anything.
    return 0;
    }

  if (dsa->GetAbstractArray(demangled_name))
    {
    // demangled_name is present, implies that component extraction is needed.
    return this->ExtractComponent(dsa, requested_name,
      demangled_name, demagled_component_name);
    }

  if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    if (cellData->GetAbstractArray(requested_name) ||
      cellData->GetAbstractArray(demangled_name))
      {
      this->CellDataToPointData(output);
      }
    }
  else if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
    if (pointData->GetAbstractArray(requested_name) ||
      pointData->GetAbstractArray(demangled_name))
      {
      this->PointDataToCellData(output);
      }
    }

  if (dsa->GetAbstractArray(requested_name))
    {
    // after the conversion the requested array is present. Don't bother doing
    // anything more.
    return 1;
    }

  if (dsa->GetAbstractArray(demangled_name))
    {
    // demangled_name is present, implies that component extraction is needed.
    return this->ExtractComponent(dsa, requested_name,
      demangled_name, demagled_component_name);
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkPVPostFilter::CellDataToPointData(vtkDataSet* output)
{
  vtkDataObject* clone = output->NewInstance();
  clone->ShallowCopy(output);

  vtkCellDataToPointData *converter = vtkCellDataToPointData::New();
  converter->SetInput(clone);
  converter->PassCellDataOn();
  converter->Update();
  output->ShallowCopy(converter->GetOutputDataObject(0));
  converter->Delete();
  clone->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPostFilter::PointDataToCellData(vtkDataSet* output)
{
  vtkDataObject* clone = output->NewInstance();
  clone->ShallowCopy(output);

  vtkPointDataToCellData *converter = vtkPointDataToCellData::New();
  converter->SetInput(clone);
  converter->PassPointDataOn();
  converter->Update();
  output->ShallowCopy(converter->GetOutputDataObject(0));
  converter->Delete();
  clone->Delete();
}

//----------------------------------------------------------------------------
namespace
{
  template <class T>
  void CopyComponent(T* outIter, T* inIter, int compNo)
    {
    vtkDataArray* inDa = vtkDataArray::SafeDownCast(inIter->GetArray());
    vtkIdType numTuples = inIter->GetNumberOfTuples();

    if (compNo == -1 && inDa == NULL)
      {
      compNo = 0;
      }

    if (compNo == -1)
      {
      vtkDataArray* outDa = vtkDataArray::SafeDownCast(outIter->GetArray());
      int numcomps = inIter->GetNumberOfComponents();
      for (vtkIdType cc=0; cc < numTuples; cc++)
        {
        double mag=0.0;
        double* tuple = inDa->GetTuple(cc);
        for (int comp=0; comp < numcomps; comp++)
          {
          mag += tuple[comp]*tuple[comp];
          }
        outDa->SetTuple1(cc, sqrt(mag));
        }
      }
    else
      {
      for (vtkIdType cc=0; cc < numTuples; cc++)
        {
        outIter->SetValue(cc, inIter->GetTuple(cc)[compNo]);
        }
      }
    }
}

//----------------------------------------------------------------------------
int vtkPVPostFilter::ExtractComponent(vtkDataSetAttributes* dsa,
  const char* requested_name, const char* demangled_name,
  const char* demagled_component_name)
{
  vtkAbstractArray* array = dsa->GetAbstractArray(demangled_name);
  assert(array != NULL && demangled_name && demagled_component_name);

  int cIndex = -1;
  // demagled_component_name can be a real component name OR
  // X,Y,Z for the first 3 components OR
  // 0,...N i.e. an integer for the index OR
  // Magnitude to indicate vector magnitude.
  // Now to the trick is to decide what way this particular request has been
  // made.
  for (int cc=0; cc < array->GetNumberOfComponents(); cc++)
    {
    const char* comp_name = array->GetComponentName(cc);
    if (comp_name && strcmp(comp_name, demagled_component_name) == 0)
      {
      cIndex = cc;
      break;
      }
    }
  if (cIndex == -1)
    {
    const char* default_names[3];
    default_names[0] = "x";
    default_names[1] = "y";
    default_names[2] = "z";
    for (int cc=0; cc < 3; cc++)
      {
      if (vtksys::SystemTools::Strucmp(demagled_component_name, default_names[cc]) == 0)
        {
        cIndex = cc;
        break;
        }
      }
    }
  if (cIndex == -1)
    {
    if (vtksys::SystemTools::Strucmp(demagled_component_name, "Magnitude") == 0)
      {
      // -1 implies magnitude.
      }
    else
      {
      cIndex = atoi(demagled_component_name);
      }
    }

  vtkAbstractArray* newArray = array->NewInstance();
  newArray->SetNumberOfComponents(1);
  newArray->SetNumberOfTuples(array->GetNumberOfTuples());
  newArray->SetName(requested_name);

  vtkArrayIterator* inIter = array->NewIterator();
  vtkArrayIterator* outIter = newArray->NewIterator();
  switch (array->GetDataType())
    {
    vtkArrayIteratorTemplateMacro(
      ::CopyComponent(static_cast<VTK_TT*>(outIter),
        static_cast<VTK_TT*>(inIter), cIndex);
    );
    }
  inIter->Delete();
  outIter->Delete();
  dsa->AddArray(newArray);
  newArray->FastDelete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVPostFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
