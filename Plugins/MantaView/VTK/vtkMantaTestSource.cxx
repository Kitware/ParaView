/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMantaTestSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMantaTestSource - produce triangles to benchmark manta with

#include "vtkMantaTestSource.h"
#include "vtkObjectFactory.h"

#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkMantaTestSource);

//----------------------------------------------------------------------------
vtkMantaTestSource::vtkMantaTestSource()
{
  this->SetNumberOfInputPorts(0);
  this->Resolution = 100;

  // Give it some geometric coherence
  this->DriftFactor = 0.1;
  // Give it some memory coherence
  this->SlidingWindow = 0.01;
}

//----------------------------------------------------------------------------
vtkMantaTestSource::~vtkMantaTestSource()
{
}

//----------------------------------------------------------------------------
void vtkMantaTestSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Resolution: " << this->Resolution << endl;
  os << indent << "DriftFactor: " << this->DriftFactor << endl;
  os << indent << "SlidingWindow: " << this->SlidingWindow << endl;
}

//----------------------------------------------------------------------------
int vtkMantaTestSource::RequestInformation(vtkInformation* vtkNotUsed(info),
  vtkInformationVector** vtkNotUsed(inputV), vtkInformationVector* output)
{
  vtkInformation* outInfo = output->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkMantaTestSource::RequestData(vtkInformation* vtkNotUsed(info),
  vtkInformationVector** vtkNotUsed(inputV), vtkInformationVector* output)
{
  vtkInformation* outInfo = output->GetInformationObject(0);
  int Rank = 0;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
  {
    Rank = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  }
  int Processors = 1;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
  {
    Processors = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  }

  // cerr << "I am " << Rank << "/" << Processors << endl;

  vtkPolyData* outPD = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!outPD)
  {
    return VTK_ERROR;
  }

  outPD->Initialize();
  outPD->Allocate();

  vtkIdType myStart = this->Resolution / Processors * Rank;
  vtkIdType myEnd = this->Resolution / Processors * (Rank + 1);

  // cerr << "I produce " << myStart << " to " << myEnd << endl;

  // TODO: Give each processor a different slice of the triangles
  vtkIdType indices[3];
  vtkIdType minIndex = this->Resolution;
  vtkIdType maxIndex = 0;
  for (vtkIdType i = 0; i < this->Resolution; i++)
  {
    double offset;
    indices[0] = -1;
    indices[1] = -2;
    indices[2] = -3;

    for (vtkIdType c = 0; c < 3; c++)
    {
      // TODO: Sliding window should be a percentage
      offset = vtkMath::Random() * this->SlidingWindow * this->Resolution -
        (this->SlidingWindow * this->Resolution / 2.0);
      indices[c] = ((vtkIdType)((double)i + c + offset));
      // don't wrap around, because can't limit ranges per processor
      // but don't restrict to strictly within my local window either
      // because otherwise geometric would change with #processors
      if (indices[c] < 0 || indices[c] >= this->Resolution)
      {
        // cerr << "BOUNCE " << indices[c] << " ";
        indices[c] = ((vtkIdType)((double)i + c - offset));
        // cerr << indices[c] << endl;
      }

      // don't make degenerate triangles
      if (indices[0] == indices[1] || indices[0] == indices[2] || indices[2] == indices[1])
      {
        // cerr << "REJECT " << i << " "
        //  << indices[0] << " " << indices[1] << " " << indices[2] << endl;
        c--;
      }
    }

    if (i >= myStart && i < myEnd)
    {
      // remember index range for this slice so we can readjust
      for (int c = 0; c < 3; c++)
      {
        if (indices[c] < minIndex)
        {
          minIndex = indices[c];
        }
        if (indices[c] > maxIndex)
        {
          maxIndex = indices[c];
        }
      }

      outPD->InsertNextCell(VTK_TRIANGLE, 3, indices);
      // cerr << "TRI " << i << " "
      //  << indices[0] << " " << indices[1] << " " << indices[2] << endl;
    }
    if (i % (this->Resolution / 10) == 0)
    {
      double frac = (double)i / this->Resolution * 0.33;
      this->UpdateProgress(frac);
      // cerr << frac << endl;
    }
  }

  // cerr << "I refer to verts between "
  //  << minIndex << " and " << maxIndex << endl;

  // shift indices to 0, because each processor only produces local points
  vtkCellArray* polys = outPD->GetPolys();
  polys->InitTraversal();
  vtkIdType npts;
  vtkIdType* thePts;
  vtkIdType i = 0;
  vtkIdType nCells = polys->GetNumberOfCells();
  while (polys->GetNextCell(npts, thePts))
  {
    for (vtkIdType c = 0; c < npts; c++)
    {
      thePts[c] = thePts[c] - minIndex;
    }
    i++;
    if (i % (nCells / 10) == 0)
    {
      double frac = (double)i / nCells * 0.33 + 0.33;
      this->UpdateProgress(frac);
      // cerr << frac << endl;
    }
  }

  vtkPoints* pts = vtkPoints::New();
  double X = vtkMath::Random();
  double Y = vtkMath::Random();
  double Z = vtkMath::Random();
  for (i = 0; (i < this->Resolution || i <= maxIndex); i++)
  {
    X = X + vtkMath::Random() * this->DriftFactor - this->DriftFactor * 0.5;
    Y = Y + vtkMath::Random() * this->DriftFactor - this->DriftFactor * 0.5;
    Z = Z + vtkMath::Random() * this->DriftFactor - this->DriftFactor * 0.5;
    if (i >= minIndex && i <= maxIndex)
    {
      // cerr << "PT " << i << " @ " << X <<","<< Y << "," << Z << endl;
      pts->InsertNextPoint(X, Y, Z);
    }

    if (i % (this->Resolution / 10) == 0)
    {
      double frac = (double)i / this->Resolution * 0.33 + 0.66;
      this->UpdateProgress(frac);
      // cerr << frac << endl;
    }
  }
  outPD->SetPoints(pts);
  pts->Delete();

  // cerr << "DONE" << endl;
  // TODO: Add attributes

  return 1;
}
