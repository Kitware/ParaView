/*=========================================================================

  Program:   ParaView
  Module:    vtkInhibitPoints.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkInhibitPoints.h"

#include "vtkMath.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkInhibitPoints);

//----------------------------------------------------------------------------
vtkInhibitPoints::vtkInhibitPoints()
{
  this->ForwardScale = 1.0;
  this->BackwardScale = 0.1;
  this->LateralScale = 0.2;
  this->Scale = 1.0;
  this->MagnitudeThreshold = 0.0;

  this->GenerateVertices = 0;
}

//----------------------------------------------------------------------------
void vtkInhibitPoints::Execute()
{
  vtkPoints *newPts;
  vtkPointData *pd;
  float *x, *xp, *v;
  float mv2, rp2, d2;
  int ptId, id, j;
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkDataSet *input= this->GetInput();
  int numPts=input->GetNumberOfPoints();
  vtkFloatArray *passedRad2 = vtkFloatArray::New();
  int numPassed = 0;
  int passedFlag;
  float tmp, thresh2;
  vtkVectors *inVects = input->GetPointData()->GetVectors();

  if (inVects == NULL)
    {
    vtkWarningMacro("No Vectors for suppression.");
    passedRad2->Delete();
    return;
    }

  thresh2 = this->MagnitudeThreshold * this->MagnitudeThreshold;

  // Check input
  //
  vtkDebugMacro(<<"Masking points");

  if ( numPts < 1 )
    {
    vtkErrorMacro(<<"No data to mask!");
    return;
    }

  // Assume ~1/4 points pass.
  passedRad2->Allocate(numPts/4);

  pd = input->GetPointData();
  id = 0;
  
  // Allocate space
  //
  newPts = vtkPoints::New();
  newPts->Allocate(numPts/4);
  outputPD->CopyAllocate(pd);

  // Traverse points and copy
  //
  int abort=0;
  int progressInterval=numPts/20 +1;


  for (ptId = 0; ptId < numPts && numPassed < 10000 && !abort;  ++ptId)
    {
    // Perform check.
    x =  input->GetPoint(ptId);
    passedFlag = 1;
    v = inVects->GetVector(ptId);
    mv2 = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
    if (mv2 < thresh2)
      {
      passedFlag = 0;
      }

    for (j = 0; j < numPassed && passedFlag; ++j)
      {
      // Get center and radius of inhibition sphere. (From previous point).
      xp = newPts->GetPoint(j);
      rp2 = passedRad2->GetValue(j);
      // Get the distance squared between the two points.
      tmp = x[0] - xp[0];
      d2 = tmp*tmp;
      tmp = x[1] - xp[1];
      d2 += tmp*tmp;
      tmp = x[2] - xp[2];
      d2 += tmp*tmp;
      if (d2 < rp2)
        { // The point failed this test.
        passedFlag = 0;
        }
      }
    if (passedFlag)
      { // The new point passed all the tests.
      // It was not inhibited by any points that passed already.
      // Save information used to suppress new points.        
      passedRad2->InsertNextValue(mv2 * this->Scale * this->Scale);
      ++numPassed;
      // Now update the output stuff.
      id = newPts->InsertNextPoint(x);
      outputPD->CopyData(pd,ptId,id);
      }

    if ( ! (id % progressInterval) ) //abort/progress
      {
      this->UpdateProgress (0.5*id/numPts);
      abort = this->GetAbortExecute();
      }
    }
      
  // Generate vertices if requested
  //
  if ( this->GenerateVertices )
    {
    vtkCellArray *verts = vtkCellArray::New();
    verts->Allocate(verts->EstimateSize(1,numPassed));
    verts->InsertNextCell(numPassed);
    for ( ptId=0; ptId<numPassed && !abort; ptId++)
      {
      if ( ! (ptId % progressInterval) ) //abort/progress
        {
        this->UpdateProgress (0.5+0.5*ptId/(id+1));
        abort = this->GetAbortExecute();
        }
      verts->InsertCellPoint(ptId);
      }
    output->SetVerts(verts);
    verts->Delete();
    }

  // Update ourselves
  //
  output->SetPoints(newPts);
  newPts->Delete();
  
  output->Squeeze();
  passedRad2->Delete();
}


//----------------------------------------------------------------------------
void vtkInhibitPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Generate Vertices: " 
     << (this->GenerateVertices ? "On\n" : "Off\n");
  os << indent << "ForwardScale: " 
     << this->ForwardScale << "\n";
  os << indent << "BackwardScale: " 
     << this->BackwardScale << "\n";
  os << indent << "LateralScale: " 
     << this->LateralScale << "\n";
  os << indent << "Scale: " 
     << this->Scale << "\n";
  os << indent << "MagnitudeThreshold: " << this->MagnitudeThreshold << "\n";
}


