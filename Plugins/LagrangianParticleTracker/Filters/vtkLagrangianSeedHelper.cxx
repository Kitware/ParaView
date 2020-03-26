/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangianSeedHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLagrangianSeedHelper.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLagrangianBasicIntegrationModel.h"
#include "vtkLagrangianParticle.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

#include <vector>

vtkStandardNewMacro(vtkLagrangianSeedHelper);

class vtkLagrangianSeedHelper::vtkInternals
{
public:
  vtkInternals() { this->CompositeDataIterator = nullptr; }

  ~vtkInternals()
  {
    if (this->CompositeDataIterator)
    {
      this->CompositeDataIterator->Delete();
    }
  }

  vtkCompositeDataIterator* CompositeDataIterator;

  typedef struct ArrayVal
  {
    std::string ArrayName;
    int Type;
    int FlowOrConstant;
    int NumberOfComponents;
    std::vector<double> Constants;
    int FlowFieldAssociation;
    std::string FlowArray;
  } ArrayVal;
  std::vector<ArrayVal> ArraysToGenerate;
};

//---------------------------------------------------------------------------
vtkLagrangianSeedHelper::vtkLagrangianSeedHelper()
{
  this->Internals = new vtkInternals();
  this->SetNumberOfInputPorts(2);
}

//---------------------------------------------------------------------------
vtkLagrangianSeedHelper::~vtkLagrangianSeedHelper()
{
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkLagrangianSeedHelper::SetSourceConnection(vtkAlgorithmOutput* algInput)
{
  this->SetInputConnection(1, algInput);
}

//---------------------------------------------------------------------------
void vtkLagrangianSeedHelper::SetSourceData(vtkDataObject* source)
{
  this->SetInputData(1, source);
}

//---------------------------------------------------------------------------
int vtkLagrangianSeedHelper::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the input and output objects
  vtkDataObject* input = vtkDataObject::GetData(inputVector[1]);
  vtkCompositeDataSet* hdInput = vtkCompositeDataSet::SafeDownCast(input);
  vtkDataSet* output = vtkDataSet::GetData(outputVector);

  if (hdInput)
  {
    // Composite data, recover the correct dataset, set during request data object pass
    input = hdInput->GetDataSet(this->Internals->CompositeDataIterator);
  }

  // Recover flow if any
  vtkDataObject* flow = vtkDataObject::GetData(inputVector[0]);

  // Clear previously setup flow
  this->IntegrationModel->ClearDataSets();

  // Check flow dataset type
  vtkCompositeDataSet* hdFlow = vtkCompositeDataSet::SafeDownCast(flow);
  vtkDataSet* dsFlow = vtkDataSet::SafeDownCast(flow);
  if (hdFlow)
  {
    // Composite data
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(hdFlow->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
      {
        // Add each leaf to the integration model
        this->IntegrationModel->AddDataSet(ds);
      }
    }
  }
  else if (dsFlow)
  {
    // Add dataset to integration model
    this->IntegrationModel->AddDataSet(dsFlow);
  }
  else
  {
    vtkErrorMacro("Ignored Flow input of type: " << (flow ? flow->GetClassName() : "(none)"));
    return 0;
  }

  // Copy input into output
  output->ShallowCopy(input);

  // Recover point data
  vtkPointData* outputPD = output->GetPointData();

  for (size_t i = 0; i < this->Internals->ArraysToGenerate.size(); i++)
  {
    vtkInternals::ArrayVal& arrayVal = this->Internals->ArraysToGenerate[i];
    vtkDataArray* seedArray = vtkDataArray::CreateDataArray(arrayVal.Type);
    seedArray->SetName(arrayVal.ArrayName.c_str());
    seedArray->SetNumberOfComponents(arrayVal.NumberOfComponents);
    seedArray->SetNumberOfTuples(output->GetNumberOfPoints());
    if (arrayVal.FlowOrConstant == vtkLagrangianSeedHelper::CONSTANT)
    {
      // Constants filling
      for (int j = 0; j < arrayVal.NumberOfComponents; j++)
      {
        seedArray->FillComponent(j, arrayVal.Constants[j]);
      }
    }
    else
    {
      // Flow data
      vtkIdType cellId;
      vtkDataSet* dataset;
      vtkAbstractCellLocator* loc;
      std::vector<double> weights(this->IntegrationModel->GetWeightsSize());
      double* weightsPtr = weights.data();

      // Create and set a dummy particle so FindInLocators can use caching.
      vtkLagrangianParticle dummyParticle(0, 0, 0, 0, 0, nullptr, 0);
      for (int iPt = 0; iPt < output->GetNumberOfPoints(); iPt++)
      {
        if (this->IntegrationModel->FindInLocators(
              output->GetPoint(iPt), &dummyParticle, dataset, cellId, loc, weightsPtr))
        {
          if (arrayVal.FlowFieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
          {
            // Cell data do not need interpolation
            vtkCellData* cd = dataset->GetCellData();
            vtkDataArray* flowArray = cd->GetArray(arrayVal.FlowArray.c_str());
            if (!flowArray)
            {
              vtkErrorMacro("Could not find " << arrayVal.FlowArray.c_str()
                                              << " array in flow cell data. Aborting");
              return 0;
            }
            if (flowArray->GetNumberOfComponents() != arrayVal.NumberOfComponents)
            {
              vtkErrorMacro(
                << arrayVal.FlowArray.c_str()
                << " cell data flow array does not have the right number of components. Aborting");
              return 0;
            }
            seedArray->SetTuple(iPt, flowArray->GetTuple(cellId));
          }
          else // if (arrayVal.FlowFieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
          {
            // PointData need interpolation
            vtkPointData* pd = dataset->GetPointData();
            vtkDataArray* flowArray = pd->GetArray(arrayVal.FlowArray.c_str());
            if (!flowArray)
            {
              vtkErrorMacro("Could not find " << arrayVal.FlowArray.c_str()
                                              << " array in flow point data. Aborting");
              return 0;
            }
            if (flowArray->GetNumberOfComponents() != arrayVal.NumberOfComponents)
            {
              vtkErrorMacro(
                << arrayVal.FlowArray.c_str()
                << " point data flow array does not have the right number of components. Aborting");
              return 0;
            }
            vtkDataArray* tmpArray = flowArray->NewInstance();
            tmpArray->SetNumberOfComponents(arrayVal.NumberOfComponents);
            tmpArray->SetNumberOfTuples(1);
            tmpArray->InterpolateTuple(
              0, dataset->GetCell(cellId)->GetPointIds(), flowArray, weightsPtr);
            seedArray->SetTuple(iPt, tmpArray->GetTuple(0));
            tmpArray->Delete();
          }
        }
        else
        {
          // Default value is zero
          for (int j = 0; j < arrayVal.NumberOfComponents; j++)
          {
            seedArray->SetComponent(iPt, j, 0);
          }
        }
      }
    }
    outputPD->AddArray(seedArray);
    seedArray->Delete();
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkLagrangianSeedHelper::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[1]);
  if (input)
  {
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());

    vtkCompositeDataSet* hdInput = vtkCompositeDataSet::SafeDownCast(input);
    vtkDataSet* actualInput = vtkDataSet::SafeDownCast(input);
    if (hdInput)
    {
      // Composite data
      if (this->Internals->CompositeDataIterator)
      {
        this->Internals->CompositeDataIterator->Delete();
      }
      this->Internals->CompositeDataIterator = hdInput->NewIterator();

      for (this->Internals->CompositeDataIterator->InitTraversal();
           !this->Internals->CompositeDataIterator->IsDoneWithTraversal();
           this->Internals->CompositeDataIterator->GoToNextItem())
      {
        vtkDataSet* ds =
          vtkDataSet::SafeDownCast(this->Internals->CompositeDataIterator->GetCurrentDataObject());
        if (ds)
        {
          // We take only the first block
          actualInput = ds;
          break;
        }
      }
    }

    if (!output || !output->IsA(actualInput->GetClassName()))
    {
      // Create output from actualInput
      vtkDataObject* newOutput = actualInput->NewInstance();
      info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkLagrangianSeedHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkLagrangianSeedHelper::RemoveAllArraysToGenerate()
{
  this->Internals->ArraysToGenerate.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkLagrangianSeedHelper::SetNumberOfArrayToGenerate(int numberOfArrays)
{
  this->Internals->ArraysToGenerate.resize(numberOfArrays);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkLagrangianSeedHelper::SetArrayToGenerate(int index, const char* arrayName, int type,
  int flowOrConstant, int numberOfComponents, const char* arrayValues)
{
  vtkInternals::ArrayVal arrayVal;
  arrayVal.ArrayName = arrayName;
  arrayVal.Type = type;
  arrayVal.FlowOrConstant = flowOrConstant;
  arrayVal.NumberOfComponents = numberOfComponents;
  if (flowOrConstant == vtkLagrangianSeedHelper::CONSTANT)
  {
    arrayVal.Constants.resize(numberOfComponents, 0);
    this->ParseDoubleValues(arrayValues, numberOfComponents, &arrayVal.Constants[0]);
  }
  else
  {
    char* tmp;
    arrayVal.FlowFieldAssociation = static_cast<int>(strtol(arrayValues, &tmp, 10));
    tmp++;
    arrayVal.FlowArray = tmp;
  }

  this->Internals->ArraysToGenerate[index] = arrayVal;
  this->Modified();
}
