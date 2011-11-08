/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToRectilinearGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetToRectilinearGrid.h"

#include "vtkAppendFilter.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOnePieceExtentTranslator.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkDataSetToRectilinearGrid);

//-----------------------------------------------------------------------------
vtkDataSetToRectilinearGrid::vtkDataSetToRectilinearGrid() 
{
}

//-----------------------------------------------------------------------------
vtkDataSetToRectilinearGrid::~vtkDataSetToRectilinearGrid()
{
}

//-----------------------------------------------------------------------------
void vtkDataSetToRectilinearGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
int vtkDataSetToRectilinearGrid::FillInputPortInformation (int port, 
                                                   vtkInformation *info)
{
  this->Superclass::FillInputPortInformation(port, info);
  
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkDataSetToRectilinearGrid::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Produce dummy extents. This should work as long as no one downstream
  // cares about structured extents. This should all go away when we
  // switch to using tables.
  int extent[6] = {0,10,0,0,0,0};
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);

  // Setup ExtentTranslator so that all downstream piece requests are
  // converted to whole extent update requests, as need by the histogram filter.
  vtkStreamingDemandDrivenPipeline* sddp = 
    vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (strcmp(
      sddp->GetExtentTranslator(outInfo)->GetClassName(), 
      "vtkOnePieceExtentTranslator") != 0)
    {
    vtkExtentTranslator* et = vtkOnePieceExtentTranslator::New();
    sddp->SetExtentTranslator(outInfo, et);
    et->Delete();
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkDataSetToRectilinearGrid::RequestUpdateExtent(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // This filter changes the ExtentTranslator on the output
  // to always update whole extent on this filter, irrespective of 
  // what piece the downstream filter is requesting. Hence, we need to
  // propagate the actual extents upstream. If upstream is structured
  // data we need to use the ExtentTranslator of the input, otherwise
  // we just set the piece information. All this is taken care of
  // by SetUpdateExtent().

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkStreamingDemandDrivenPipeline* sddp = 
    vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (outInfo->Has(sddp->UPDATE_NUMBER_OF_PIECES()) && 
    outInfo->Has(sddp->UPDATE_PIECE_NUMBER()) &&
    outInfo->Has(sddp->UPDATE_NUMBER_OF_GHOST_LEVELS()))
    {
    int piece = outInfo->Get(sddp->UPDATE_PIECE_NUMBER());
    int numPieces = outInfo->Get(sddp->UPDATE_NUMBER_OF_PIECES());
    int ghostLevel = outInfo->Get(sddp->UPDATE_NUMBER_OF_GHOST_LEVELS());

    sddp->SetUpdateExtent(inInfo, piece, numPieces, ghostLevel);
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkDataSetToRectilinearGrid::RequestData(vtkInformation* /*request*/, 
                                     vtkInformationVector** inputVector, 
                                     vtkInformationVector* outputVector)
{
  vtkRectilinearGrid* output = vtkRectilinearGrid::GetData(outputVector);

  vtkDataSet* dsinput = vtkDataSet::GetData(inputVector[0], 0);
  vtkCompositeDataSet* cinput = vtkCompositeDataSet::GetData(inputVector[0], 0);
  vtkSmartPointer<vtkDataSet> input = 0;
  if (dsinput)
    {
    input = dsinput;
    }
  else if (cinput)
    {
    vtkSmartPointer<vtkAppendFilter> af =
      vtkSmartPointer<vtkAppendFilter>::New();
    vtkCompositeDataIterator *cdit = cinput->NewIterator();
    cdit->InitTraversal();
    bool foundone = false;
    while(!cdit->IsDoneWithTraversal())
      {
      vtkDataSet *ds = vtkDataSet::SafeDownCast(cdit->GetCurrentDataObject());
      if (ds && ds->GetNumberOfPoints() > 0)
        {
        foundone = true;
        af->AddInputData(ds);
        }
      cdit->GoToNextItem();
      }
    cdit->Delete();
    if (foundone)
      {
      af->Update();
      }
    input = vtkDataSet::SafeDownCast(af->GetOutputDataObject(0));
    }
  if (!input)
    {
    vtkErrorMacro("Unrecognized input type: "
        << vtkDataObject::GetData(inputVector[0], 0)->GetClassName());
    return 0;
    }
  
  vtkSmartPointer<vtkDoubleArray> otherCoords = 
    vtkSmartPointer<vtkDoubleArray>::New();
  otherCoords->SetNumberOfComponents(1);
  otherCoords->SetNumberOfTuples(1);
  otherCoords->SetTuple1(0, 0.0);
  output->SetYCoordinates(otherCoords);
  output->SetZCoordinates(otherCoords);
  
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkSmartPointer<vtkDoubleArray> pts = 
    vtkSmartPointer<vtkDoubleArray>::New();
  pts->SetNumberOfTuples(numPts);
  for (vtkIdType i=0; i<numPts; i++)
    {
    pts->SetValue(i, input->GetPoint(i)[0]);
    }
    
  output->SetDimensions(numPts, 1, 1);
  
  output->GetPointData()->PassData(input->GetPointData());

  return 1;
}
