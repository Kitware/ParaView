/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInhibitPoints.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

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
vtkInhibitPoints* vtkInhibitPoints::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkInhibitPoints");
  if(ret)
    {
    return (vtkInhibitPoints*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkInhibitPoints;
}

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
  float *x, *x0, *n0;
  float m0;
  float *v, mv;
  int ptId, id, j;
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkDataSet *input= this->GetInput();
  int numPts=input->GetNumberOfPoints();
  vtkFloatArray *passed = vtkFloatArray::New();
  int numPassed = 0;
  int passedFlag;
  float tmp, majorD, minorD2;
  float lateralScaleSquared;
  vtkVectors *inVects = input->GetPointData()->GetVectors();

  if (inVects == NULL)
    {
    vtkWarningMacro("No Vectors for suppression.");
    passed->Delete();
    return;
    }

  lateralScaleSquared = this->LateralScale * this->Scale;
  lateralScaleSquared = lateralScaleSquared * lateralScaleSquared;

  // Check input
  //
  vtkDebugMacro(<<"Masking points");

  if ( numPts < 1 )
    {
    vtkErrorMacro(<<"No data to mask!");
    return;
    }

  // Assume ~1/4 points pass. 7 floats per passes point are kept.
  passed->Allocate(numPts*2);

  pd = input->GetPointData();
  id = 0;
  
  // Allocate space
  //
  newPts = vtkPoints::New();
  newPts->Allocate(numPts);
  outputPD->CopyAllocate(pd);

  // Traverse points and copy
  //
  int abort=0;
  int progressInterval=numPts/20 +1;


  for (ptId = 0; ptId < numPts && !abort;  ++ptId)
    {
    // Perform check.
    x =  input->GetPoint(ptId);
    passedFlag = 1;
    v = inVects->GetVector(ptId);
    mv = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (mv < this->MagnitudeThreshold)
      {
      passedFlag = 0;
      }

    for (j = 0; j < numPassed && passedFlag; ++j)
      {
      // Compute major/minor distances.
      x0 = passed->GetPointer(7*j);
      n0 = x0 + 3;
      m0 = n0[3];
      // Compute distance from normal and the two points.
      majorD = (x[0]-x0[0])*n0[0] + (x[1]-x0[1])*n0[1] 
                  + (x[2]-x0[2])*n0[2];
      // Compute prependicular distance squared.
      tmp = x[0] - x0[0] + n0[0]*majorD;
      minorD2 = tmp * tmp;
      tmp = x[1] - x0[1] + n0[1]*majorD;
      minorD2 += tmp * tmp;
      tmp = x[2] - x0[2] + n0[2]*majorD;
      minorD2 += tmp * tmp;
      // Scale here to avoid two multiplies in condition.
      majorD = majorD * m0;
      if (majorD < this->ForwardScale*this->Scale && 
          majorD > -this->BackwardScale*this->Scale &&
          minorD2 * m0 * m0 < lateralScaleSquared)
        { // The point failed this test.
        passedFlag = 0;
        }
      }
    if (passedFlag)
      { // The new point passed all the tests.
      // It was not inhibited by any points that passed already.
      // Save information used to suppress new points.        
      passed->InsertNextValue(x[0]);
      passed->InsertNextValue(x[1]);
      passed->InsertNextValue(x[2]);
      passed->InsertNextValue(v[0]/mv);
      passed->InsertNextValue(v[1]/mv);
      passed->InsertNextValue(v[2]/mv);
      passed->InsertNextValue(mv);
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
    verts->Allocate(verts->EstimateSize(1,id+1));
    verts->InsertNextCell(id+1);
    for ( ptId=0; ptId<(id+1) && !abort; ptId++)
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


