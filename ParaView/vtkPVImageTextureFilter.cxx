/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageTextureFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVImageTextureFilter.h"
#include "vtkPlaneSource.h"
#include "vtkObjectFactory.h"
#include "vtkImageClip.h"


//------------------------------------------------------------------------------
vtkPVImageTextureFilter* vtkPVImageTextureFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVImageTextureFilter");
  if(ret)
    {
    return (vtkPVImageTextureFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVImageTextureFilter;
}




//----------------------------------------------------------------------------
vtkPVImageTextureFilter::vtkPVImageTextureFilter ()
{
  this->vtkSource::SetNthOutput(0, vtkPolyData::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
  
  this->vtkSource::SetNthOutput(1, vtkImageData::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[1]->ReleaseData();
  this->Outputs[1]->Delete();
  
  this->IntermediateImage = vtkImageData::New();
  this->Clip = vtkImageClip::New();
  this->Clip->SetInput(this->IntermediateImage);
  this->Clip->ClipDataOn();
  
  this->PlaneSource = vtkPlaneSource::New();
  this->PlaneSource->SetXResolution(1);
  this->PlaneSource->SetYResolution(1);

  this->Assignment = NULL;
  
  this->Extent[0] = this->Extent[1] = this->Extent[2] = 0;
  this->Extent[3] = this->Extent[4] = this->Extent[5] = 0;  

  this->PlaneAxis = 0;
}

//----------------------------------------------------------------------------
vtkPVImageTextureFilter::~vtkPVImageTextureFilter ()
{
  this->IntermediateImage->Delete();
  this->IntermediateImage = NULL;

  this->Clip->Delete();
  this->Clip = NULL;

  this->PlaneSource->Delete();
  this->PlaneSource = NULL;
  
  this->SetAssignment(NULL);
}

//----------------------------------------------------------------------------
void vtkPVImageTextureFilter::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkPVImageTextureFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[0]);
}

//----------------------------------------------------------------------------
void vtkPVImageTextureFilter::SetGeometryOutput(vtkPolyData *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
vtkPolyData *vtkPVImageTextureFilter::GetGeometryOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkPolyData *)(this->Outputs[0]);
}

//----------------------------------------------------------------------------
void vtkPVImageTextureFilter::SetTextureOutput(vtkImageData *output)
{
  this->vtkSource::SetNthOutput(1, output);
}

//----------------------------------------------------------------------------
vtkImageData *vtkPVImageTextureFilter::GetTextureOutput()
{
  if (this->NumberOfOutputs < 2)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Outputs[1]);
}

//----------------------------------------------------------------------------
void vtkPVImageTextureFilter::ComputeInputUpdateExtents( vtkDataObject *o)
{
  vtkImageData *input = this->GetInput();
  int *aExt;
  o = o;

  input->GetWholeExtent(this->Extent);

  vtkDebugMacro("WholeExtent: " << this->Extent[0] << ", " << this->Extent[1] << ", "
		<< this->Extent[2] << ", " << this->Extent[3] << ", "
		<< this->Extent[4] << ", " << this->Extent[5]);
  
  if (this->Assignment)
    {
    this->Assignment->SetWholeExtent(this->Extent);
    aExt = this->Assignment->GetExtent();
    
    vtkDebugMacro(<< this->Assignment << " AssignExtent: " << aExt[0] << ", " 
    << aExt[1] << ", " << aExt[2] << ", " << aExt[3] << ", " << aExt[4] << ", " 
    << aExt[5]);
    
    if (aExt[0] > this->Extent[0])
      {
      this->Extent[0] = aExt[0];
      }
    if (aExt[1] < this->Extent[1])
      {
      this->Extent[1] = aExt[1];
      }
    
    if (aExt[2] > this->Extent[2])
      {
      this->Extent[2] = aExt[2];
      }
    if (aExt[3] < this->Extent[3])
      {
      this->Extent[3] = aExt[3];
      }
    
    if (aExt[4] > this->Extent[4])
      {
      this->Extent[4] = aExt[4];
      }
    if (aExt[5] < this->Extent[5])
      {
      this->Extent[5] = aExt[5];
      }
    }
  
  // Check for condition of no overlap
  if (this->Extent[0] > this->Extent[1] || 
      this->Extent[2] > this->Extent[3] ||
      this->Extent[4] > this->Extent[5])
    {
    this->Extent[0] = this->Extent[2] = this->Extent[4] = 0;
    this->Extent[1] = this->Extent[3] = this->Extent[5] = -1;    
    }
  
  vtkDebugMacro("Extent: " << this->Extent[0] << ", " << this->Extent[1] << ", "
		<< this->Extent[2] << ", " << this->Extent[3] << ", "
		<< this->Extent[4] << ", " << this->Extent[5]);
  
  input->SetUpdateExtent(this->Extent);
}

#include "vtkStructuredPointsWriter.h"


//----------------------------------------------------------------------------
void vtkPVImageTextureFilter::Execute()
{
  float x, y, z;
  float *spacing;
  float *origin;
  vtkImageData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkImageData *outputTexture = this->GetTextureOutput();
  
  if (this->Extent[0] > this->Extent[1])
    {
    return;
    }
  
  this->IntermediateImage->ShallowCopy(input);
  this->Clip->SetOutputWholeExtent(this->Extent);
  this->Clip->Update();
  
  outputTexture->ShallowCopy(this->Clip->GetOutput());
  vtkDebugMacro(<< *outputTexture);
  vtkStructuredPointsWriter *w;
  w = vtkStructuredPointsWriter::New();
  w->SetInput(this->IntermediateImage);
  w->SetFileTypeToBinary();
  w->SetFileName("junkTexture.vtk");
  //w->Write();
  w->Delete();
  
  // Determine which axis we are displaying.
  if (this->Extent[0] == this->Extent[1])
    {
    this->PlaneAxis = 0;
    }
  else if (this->Extent[2] == this->Extent[3])
    {
    this->PlaneAxis = 1;
    }
  else if (this->Extent[4] == this->Extent[5])
    {
    this->PlaneAxis = 2;
    }
  else
    {
    vtkErrorMacro("We do not handle volumes at this time.");
    }
  
  spacing = input->GetSpacing();
  origin = input->GetOrigin();
  
  // Place the plane in the correct position.
  x = origin[0] + (float)(this->Extent[0]) * spacing[0];
  y = origin[1] + (float)(this->Extent[2]) * spacing[1];
  z = origin[2] + (float)(this->Extent[4]) * spacing[2];

  vtkDebugMacro("Origin: " << x << ", " << y << ", " << z);
  this->PlaneSource->SetOrigin(x, y, z);

  if (this->PlaneAxis == 0)
    {
    x = origin[0] + (float)(this->Extent[0]) * spacing[0];
    y = origin[1] + (float)(this->Extent[3]) * spacing[1];
    z = origin[2] + (float)(this->Extent[4]) * spacing[2];
    this->PlaneSource->SetPoint1(x, y, z);
    x = origin[0] + (float)(this->Extent[0]) * spacing[0];
    y = origin[1] + (float)(this->Extent[2]) * spacing[1];
    z = origin[2] + (float)(this->Extent[5]) * spacing[2];
    this->PlaneSource->SetPoint2(x, y, z);
    }
  if (this->PlaneAxis == 1)
    {
    x = origin[0] + (float)(this->Extent[1]) * spacing[0];
    y = origin[1] + (float)(this->Extent[2]) * spacing[1];
    z = origin[2] + (float)(this->Extent[4]) * spacing[2];
    this->PlaneSource->SetPoint1(x, y, z);
    x = origin[0] + (float)(this->Extent[0]) * spacing[0];
    y = origin[1] + (float)(this->Extent[2]) * spacing[1];
    z = origin[2] + (float)(this->Extent[5]) * spacing[2];
    this->PlaneSource->SetPoint2(x, y, z);
    }  
  if (this->PlaneAxis == 2)
    {
    x = origin[0] + (float)(this->Extent[1]) * spacing[0];
    y = origin[1] + (float)(this->Extent[2]) * spacing[1];
    z = origin[2] + (float)(this->Extent[4]) * spacing[2];
    
    vtkDebugMacro("P1: " << x << ", " << y << ", " << z);
    
    this->PlaneSource->SetPoint1(x, y, z);
    x = origin[0] + (float)(this->Extent[0]) * spacing[0];
    y = origin[1] + (float)(this->Extent[3]) * spacing[1];
    z = origin[2] + (float)(this->Extent[4]) * spacing[2];
    this->PlaneSource->SetPoint2(x, y, z);

   vtkDebugMacro("P2: " << x << ", " << y << ", " << z);
    
    }
  
  this->PlaneSource->Update();
  output->ShallowCopy(this->PlaneSource->GetOutput());
}


