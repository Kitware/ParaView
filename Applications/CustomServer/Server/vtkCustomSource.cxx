/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkCustomSource.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include <math.h>

vtkCxxRevisionMacro(vtkCustomSource, "1.1");
vtkStandardNewMacro(vtkCustomSource);

vtkCustomSource::vtkCustomSource(double xL, double yL, double zL)
{
  this->XLength = fabs(xL);
  this->YLength = fabs(yL);
  this->ZLength = fabs(zL);

  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;

  this->SetNumberOfInputPorts(0);
}

int vtkCustomSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  double x[3], n[3], tc[3];
  int numPolys=6, numPts=24;
  int i, j, k;
  vtkIdType pts[4];
  vtkPoints *newPoints; 
  vtkFloatArray *newNormals;
  vtkFloatArray *newTCoords; // CCS 7/27/98 Added for Texture Mapping
  vtkCellArray *newPolys;
  
//
// Set things up; allocate memory
//
  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);
  newNormals = vtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(numPts);
  newNormals->SetName("Normals");
  newTCoords = vtkFloatArray::New();
  newTCoords->SetNumberOfComponents(2);
  newTCoords->Allocate(numPts);
  newTCoords->SetName("TCoords");

  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(numPolys,4));
//
// Generate points and normals
//

  for (x[0]=this->Center[0]-this->XLength/2.0, n[0]=(-1.0), n[1]=n[2]=0.0, i=0; 
  i<2; i++, x[0]+=this->XLength, n[0]+=2.0)
    {
    for (x[1]=this->Center[1]-this->YLength/2.0, j=0; j<2; 
    j++, x[1]+=this->YLength)
      {
      tc[1] =  x[1] + 0.5;
      for (x[2]=this->Center[2]-this->ZLength/2.0, k=0; k<2; 
      k++, x[2]+=this->ZLength)
        {
        tc[0] = (x[2] + 0.5) * ( 1 - 2*i );
        newPoints->InsertNextPoint(x);
        newTCoords->InsertNextTuple(tc);
        newNormals->InsertNextTuple(n);
        }
      }
    }
  pts[0] = 0; pts[1] = 1; pts[2] = 3; pts[3] = 2; 
  newPolys->InsertNextCell(4,pts);
  pts[0] = 4; pts[1] = 6; pts[2] = 7; pts[3] = 5; 
  newPolys->InsertNextCell(4,pts);

  for (x[1]=this->Center[1]-this->YLength/2.0, n[1]=(-1.0), n[0]=n[2]=0.0, i=0; 
  i<2; i++, x[1]+=this->YLength, n[1]+=2.0)
    {
    for (x[0]=this->Center[0]-this->XLength/2.0, j=0; j<2; 
    j++, x[0]+=this->XLength)
      {
      tc[0] = ( x[0] + 0.5 ) * ( 2*i - 1 );
      for (x[2]=this->Center[2]-this->ZLength/2.0, k=0; k<2; 
      k++, x[2]+=this->ZLength)
        {
        tc[1] = ( x[2] + 0.5 ) * -1;
        newPoints->InsertNextPoint(x);
        newTCoords->InsertNextTuple(tc);
        newNormals->InsertNextTuple(n);
        }
      }
    }
  pts[0] = 8; pts[1] = 10; pts[2] = 11; pts[3] = 9; 
  newPolys->InsertNextCell(4,pts);
  pts[0] = 12; pts[1] = 13; pts[2] = 15; pts[3] = 14; 
  newPolys->InsertNextCell(4,pts);

  for (x[2]=this->Center[2]-this->ZLength/2.0, n[2]=(-1.0), n[0]=n[1]=0.0, i=0; 
  i<2; i++, x[2]+=this->ZLength, n[2]+=2.0)
    {
    for (x[1]=this->Center[1]-this->YLength/2.0, j=0; j<2; 
    j++, x[1]+=this->YLength)
      {
      tc[1] = x[1] + 0.5;
      for (x[0]=this->Center[0]-this->XLength/2.0, k=0; k<2; 
      k++, x[0]+=this->XLength)
        {
        tc[0] = ( x[0] + 0.5 ) * ( 2*i - 1 );
        newPoints->InsertNextPoint(x);
        newTCoords->InsertNextTuple(tc);
        newNormals->InsertNextTuple(n);
        }
      }
    }
  pts[0] = 16; pts[1] = 18; pts[2] = 19; pts[3] = 17; 
  newPolys->InsertNextCell(4,pts);
  pts[0] = 20; pts[1] = 21; pts[2] = 23; pts[3] = 22; 
  newPolys->InsertNextCell(4,pts);
//
// Update ourselves and release memory
//
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->GetPointData()->SetNormals(newNormals);
  newNormals->Delete();

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();

  newPolys->Squeeze(); // since we've estimated size; reclaim some space
  output->SetPolys(newPolys);
  newPolys->Delete();

  return 1;
}

// Convenience method allows creation of cube by specifying bounding box.
void vtkCustomSource::SetBounds(double xMin, double xMax,
                              double yMin, double yMax,
                              double zMin, double zMax)
{
  double bounds[6];
  bounds[0] = xMin;
  bounds[1] = xMax;
  bounds[2] = yMin;
  bounds[3] = yMax;
  bounds[4] = zMin;
  bounds[5] = zMax;
  this->SetBounds (bounds);
}

void vtkCustomSource::SetBounds(double bounds[6])
{
  this->SetXLength(bounds[1]-bounds[0]);
  this->SetYLength(bounds[3]-bounds[2]);
  this->SetZLength(bounds[5]-bounds[4]);

  this->SetCenter((bounds[1]+bounds[0])/2.0, (bounds[3]+bounds[2])/2.0, 
                  (bounds[5]+bounds[4])/2.0);
}

void vtkCustomSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "X Length: " << this->XLength << "\n";
  os << indent << "Y Length: " << this->YLength << "\n";
  os << indent << "Z Length: " << this->ZLength << "\n";
  os << indent << "Center: (" << this->Center[0] << ", " 
               << this->Center[1] << ", " << this->Center[2] << ")\n";
}
