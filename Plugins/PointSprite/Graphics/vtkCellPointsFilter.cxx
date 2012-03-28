/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellPointsFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkCellPointsFilter
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>

#include "vtkCellPointsFilter.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataIterator.h"
#include "vtkPolyData.h"
#include "vtkGenericDataSet.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkObjectFactory.h"
#include "vtkAppendPolyData.h"
#include "vtkSmartPointer.h"
//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkCellPointsFilter)
//----------------------------------------------------------------------------

vtkCellPointsFilter::vtkCellPointsFilter(void)
{
  this->VertexCells = true;
}

vtkCellPointsFilter::~vtkCellPointsFilter(void)
{

}

int vtkCellPointsFilter::FillInputPortInformation(int port,
    vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");

  return 1;
}

//---------------------------------------------------------------------------
int vtkCellPointsFilter::RequestInformation(vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);
  return 1;
}
//---------------------------------------------------------------------------
void vtkCellPointsFilter::ExecuteSimple(vtkDataSet *input, vtkPolyData *output)
{
  vtkPoints *pts = NULL;
  vtkCellArray *verts = NULL;
  int npoints;
  double pt[3];
  //
  if (!input)
    {
    vtkErrorMacro(<<"No input data");
    }
  //
  if (input->IsA("vtkPointSet"))
    {
    npoints = input->GetNumberOfPoints();
    pts = vtkPointSet::SafeDownCast(input)->GetPoints();
    output->SetPoints(pts);
    }
  else
    {
    npoints = input->GetNumberOfPoints();
    pts = vtkPoints::New();
    pts->SetNumberOfPoints(npoints);
    for (int i = 0; i < npoints; i++)
      {
      input->GetPoint(i, pt);
      pts->SetPoint(i, pt);
      }
    output->SetPoints(pts);
    pts->Delete();
    }
  //
  vtkPointData *in_PD = input->GetPointData();
  vtkPointData *out_PD = output->GetPointData();
  out_PD->PassData(in_PD);
  //
  verts = vtkCellArray::New();
  verts->Allocate(npoints * 2);
  //vtkIdType *ptr = verts->GetPointer();
  //
  for (vtkIdType i = 0; i < npoints; i++)
    {
    verts->InsertNextCell(1, &i);
    }
  output->SetVerts(verts);
  verts->Delete();
}
//----------------------------------------------------------------------------
int vtkCellPointsFilter::ExecuteCompositeDataSet(vtkCompositeDataSet* input,
    vtkAppendPolyData* append)
{
  int totInputs = 0;
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(input->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    totInputs++;
    }
  double i = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (ds)
      {
      vtkPolyData* tmpOut = vtkPolyData::New();
      this->ExecuteSimple(ds, tmpOut);
      append->AddInputData(tmpOut);
      // Call FastDelete() instead of Delete() to avoid garbage
      // collection checks. This improves the performance significantly
      tmpOut->FastDelete();
      this->UpdateProgress(static_cast<double> (i++) / totInputs);
      }
    }
  return 1;
}
//----------------------------------------------------------------------------
int vtkCellPointsFilter::RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
  // check valid output
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPolyData *output = vtkPolyData::SafeDownCast(outInfo->Get(
      vtkDataObject::DATA_OBJECT()));
  if (!output)
    {
    return 0;
    }

  // check input type
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* inputDobj = inInfo->Get(vtkDataObject::DATA_OBJECT());
  //
  vtkDataSet *input = vtkDataSet::SafeDownCast(inputDobj);
  if (input)
    {
    this->ExecuteSimple(input, output);
    return 1;
    }
  //
  vtkCompositeDataSet *cds = vtkCompositeDataSet::SafeDownCast(inputDobj);
  if (cds)
    {
    return this->RequestCompositeData(request, inputVector, outputVector);
    }

  // we can't handle other types yet
  return 0;
}

//----------------------------------------------------------------------------
int vtkCellPointsFilter::RequestCompositeData(vtkInformation*,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkPolyData *output = vtkPolyData::SafeDownCast(info->Get(
      vtkDataObject::DATA_OBJECT()));
  if (!output)
    {
    return 0;
    }

  vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(inInfo->Get(
      vtkDataObject::DATA_OBJECT()));
  if (!input)
    {
    vtkErrorMacro("This filter cannot handle input of type: "
        << inInfo->Get(vtkDataObject::DATA_OBJECT())->GetClassName());
    return 0;
    }

  if (this->CheckAttributes(input))
    {
    return 0;
    }

  vtkAppendPolyData* append = vtkAppendPolyData::New();
  //int numInputs = 0;

  int retVal = 0;
  if (this->ExecuteCompositeDataSet(input, append))
    {
    append->Update();
    output->ShallowCopy(append->GetOutput());
    append->Delete();
    retVal = 1;
    }

  return retVal;
}
//----------------------------------------------------------------------------
vtkExecutive* vtkCellPointsFilter::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}
//----------------------------------------------------------------------------
int vtkCellPointsFilter::CheckAttributes(vtkDataObject* input)
{
  if (input->IsA("vtkDataSet"))
    {
    if (static_cast<vtkDataSet*> (input)->CheckAttributes())
      {
      return 1;
      }
    }
  else if (input->IsA("vtkCompositeDataSet"))
    {
    vtkCompositeDataSet* compInput = static_cast<vtkCompositeDataSet*> (input);
    vtkCompositeDataIterator* iter = compInput->NewIterator();
    iter->GoToFirstItem();
    while (!iter->IsDoneWithTraversal())
      {
      vtkDataObject* curDataSet = iter->GetCurrentDataObject();
      if (curDataSet && this->CheckAttributes(curDataSet))
        {
        return 1;
        }
      iter->GoToNextItem();
      }
    iter->Delete();
    }
  return 0;
}

