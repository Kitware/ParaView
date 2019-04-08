/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangianSurfaceHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLagrangianSurfaceHelper.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLagrangianBasicIntegrationModel.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <vector>

vtkStandardNewMacro(vtkLagrangianSurfaceHelper);

class vtkLagrangianSurfaceHelper::vtkInternals
{
public:
  typedef struct ArrayVal
  {
    std::string ArrayName;
    int Type;
    int NumberOfLeafs;
    int NumberOfComponents;
    std::vector<std::vector<double> > Constants;
    std::vector<bool> Skips;
  } ArrayVal;
  std::vector<ArrayVal> ArraysToGenerate;
};

//---------------------------------------------------------------------------
vtkLagrangianSurfaceHelper::vtkLagrangianSurfaceHelper()
{
  this->Internals = new vtkInternals();
}

//---------------------------------------------------------------------------
vtkLagrangianSurfaceHelper::~vtkLagrangianSurfaceHelper()
{
  delete this->Internals;
}

//---------------------------------------------------------------------------
int vtkLagrangianSurfaceHelper::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  // Add input to the model so it will be able to use it to compute default values
  this->IntegrationModel->ClearDataSets(true);
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  vtkCompositeDataSet* hdInput = vtkCompositeDataSet::SafeDownCast(input);
  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(input);
  if (hdInput)
  {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(hdInput->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataSet* inDataset = vtkDataSet::SafeDownCast(hdInput->GetDataSet(iter));
      if (inDataset)
      {
        this->IntegrationModel->AddDataSet(inDataset, true, iter->GetCurrentFlatIndex());
      }
    }
  }
  else if (dsInput)
  {
    this->IntegrationModel->AddDataSet(dsInput, true);
  }
  return 1;
}

//---------------------------------------------------------------------------
int vtkLagrangianSurfaceHelper::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the input and output objects
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  vtkDataObject* output = vtkDataObject::GetData(outputVector);

  vtkCompositeDataSet* hdInput = vtkCompositeDataSet::SafeDownCast(input);
  vtkCompositeDataSet* hdOutput = vtkCompositeDataSet::SafeDownCast(output);
  vtkDataSet* dsOutput = vtkDataSet::SafeDownCast(output);
  if (hdOutput)
  {
    hdOutput->CopyStructure(hdInput);
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(hdInput->NewIterator());
    int nLeaf = 0;
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataSet* inDataset = vtkDataSet::SafeDownCast(hdInput->GetDataSet(iter));
      if (inDataset)
      {
        vtkDataSet* outDataset = inDataset->NewInstance();
        outDataset->ShallowCopy(inDataset);
        // Fill each leaf of the composite dataset
        this->FillFieldData(outDataset, nLeaf);
        hdOutput->SetDataSet(iter, outDataset);
        nLeaf++;
        outDataset->Delete();
      }
    }
  }
  else if (dsOutput)
  {
    dsOutput->ShallowCopy(input);
    // Fill single dataset
    this->FillFieldData(dsOutput, 0);
  }
  else
  {
    vtkErrorMacro("Unsupported dataset type : " << output->GetClassName());
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkLagrangianSurfaceHelper::FillFieldData(vtkDataSet* dataset, int leaf)
{
  vtkFieldData* fd = dataset->GetFieldData();

  for (size_t i = 0; i < this->Internals->ArraysToGenerate.size(); i++)
  {
    vtkInternals::ArrayVal& arrayVal = this->Internals->ArraysToGenerate[i];
    if (leaf > arrayVal.NumberOfLeafs)
    {
      vtkWarningMacro("Leaf :" << leaf << "does not exist in constants values. Ignoring.");
      return;
    }

    // Check for skip
    if (!arrayVal.Skips[leaf])
    {
      // Create the array
      vtkDataArray* surfaceArray = vtkDataArray::CreateDataArray(arrayVal.Type);
      surfaceArray->SetName(arrayVal.ArrayName.c_str());
      surfaceArray->SetNumberOfComponents(arrayVal.NumberOfComponents);
      surfaceArray->SetNumberOfTuples(1);

      // Constants filling for correct leaf
      surfaceArray->SetTuple(0, &arrayVal.Constants[leaf][0]);
      fd->AddArray(surfaceArray);
      surfaceArray->Delete();
    }
  }
}

//----------------------------------------------------------------------------
int vtkLagrangianSurfaceHelper::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Create output from input
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  if (input)
  {
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());
    if (!output || !output->IsA(input->GetClassName()))
    {
      vtkDataObject* newOutput = input->NewInstance();
      info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkLagrangianSurfaceHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  for (size_t i = 0; i < this->Internals->ArraysToGenerate.size(); i++)
  {
    vtkInternals::ArrayVal& arrayVal = this->Internals->ArraysToGenerate[i];
    os << indent << "Arrays To Generate:" << endl;
    indent = indent.GetNextIndent();
    os << indent << "Name: " << arrayVal.ArrayName << endl;
    os << indent << "Type: " << vtkImageScalarTypeNameMacro(arrayVal.Type) << endl;
    os << indent << "Number of leafs: " << arrayVal.NumberOfLeafs << endl;
    os << indent << "Number of components: " << arrayVal.NumberOfComponents << endl;
    os << indent << "Constants: ";
    for (size_t j = 0; j < arrayVal.Constants.size(); j++)
    {
      for (size_t k = 0; k < arrayVal.Constants[j].size(); k++)
      {
        os << arrayVal.Constants[j][k] << " ";
      }
    }
    os << endl;
    os << indent << "Skips: ";
    for (size_t j = 0; j < arrayVal.Skips.size(); j++)
    {
      os << arrayVal.Skips[j] << " ";
    }
    os << endl;
  }
}

//----------------------------------------------------------------------------
void vtkLagrangianSurfaceHelper::RemoveAllArraysToGenerate()
{
  this->Internals->ArraysToGenerate.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkLagrangianSurfaceHelper::SetNumberOfArrayToGenerate(int numberOfArrays)
{
  this->Internals->ArraysToGenerate.resize(numberOfArrays);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkLagrangianSurfaceHelper::SetArrayToGenerate(int index, const char* arrayName, int type,
  int numberOfLeafs, int numberOfComponents, const char* arrayValues)
{
  vtkInternals::ArrayVal arrayVal;
  arrayVal.ArrayName = arrayName;
  arrayVal.Type = type;
  arrayVal.NumberOfLeafs = numberOfLeafs;
  arrayVal.NumberOfComponents = numberOfComponents;
  arrayVal.Constants.resize(numberOfLeafs, std::vector<double>(numberOfComponents, 0));
  const char* constants = arrayValues;
  for (int i = 0; i < numberOfLeafs; i++)
  {
    arrayVal.Skips.push_back(
      !this->ParseDoubleValues(constants, numberOfComponents, &arrayVal.Constants[i][0]));
  }
  this->Internals->ArraysToGenerate[index] = arrayVal;
  this->Modified();
}
