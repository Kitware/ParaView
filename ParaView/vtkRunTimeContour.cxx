/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRunTimeContour.cxx
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

#include "vtkRunTimeContour.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkRunTimeContour* vtkRunTimeContour::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkRunTimeContour");
  if(ret)
    {
    return (vtkRunTimeContour*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkRunTimeContour;
}

//----------------------------------------------------------------------------
vtkRunTimeContour::vtkRunTimeContour()
{
  this->Reader = vtkStructuredPointsReader::New();
  this->Contour = vtkSingleContourFilter::New();
  this->Contour->SetInput(this->Reader->GetOutput());
  this->ContourScale = NULL;

  this->Range[0] = 0.0;
  this->Range[1] = 1.0;

  this->XClip = vtkImageClip::New();
  this->XClip->SetInput(this->Reader->GetOutput());
  this->YClip = vtkImageClip::New();
  this->YClip->SetInput(this->Reader->GetOutput());
  this->ZClip = vtkImageClip::New();
  this->ZClip->SetInput(this->Reader->GetOutput());

  this->XSlice = this->YSlice = this->ZSlice = 0;
  this->XScale = this->YScale = this->ZScale = NULL;

  this->SetXSliceOutput(vtkImageData::New());
  this->SetYSliceOutput(vtkImageData::New());
  this->SetZSliceOutput(vtkImageData::New());
}

//----------------------------------------------------------------------------
vtkRunTimeContour::~vtkRunTimeContour()
{
  this->Reader->Delete();
  this->Reader = NULL;
  this->Contour->Delete();
  this->Contour = NULL;
  this->XClip->Delete();
  this->XClip = NULL;
  this->YClip->Delete();
  this->YClip = NULL;
  this->ZClip->Delete();
  this->ZClip = NULL;

  this->SetXScale(NULL);
  this->SetYScale(NULL);
  this->SetZScale(NULL);
  this->SetContourScale(NULL);
}

//----------------------------------------------------------------------------
vtkImageData *vtkRunTimeContour::GetXSliceOutput()
{
  if (this->NumberOfOutputs < 2)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Outputs[1]);
}

//----------------------------------------------------------------------------
void vtkRunTimeContour::SetXSliceOutput(vtkImageData *output)
{
  this->vtkSource::SetNthOutput(1, output);
}

//----------------------------------------------------------------------------
vtkImageData *vtkRunTimeContour::GetYSliceOutput()
{
  if (this->NumberOfOutputs < 3)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Outputs[2]);
}

//----------------------------------------------------------------------------
void vtkRunTimeContour::SetYSliceOutput(vtkImageData *output)
{
  this->vtkSource::SetNthOutput(2, output);
}

//----------------------------------------------------------------------------
vtkImageData *vtkRunTimeContour::GetZSliceOutput()
{
  if (this->NumberOfOutputs < 4)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Outputs[3]);
}

//----------------------------------------------------------------------------
void vtkRunTimeContour::SetZSliceOutput(vtkImageData *output)
{
  this->vtkSource::SetNthOutput(3, output);
}

//----------------------------------------------------------------------------
void vtkRunTimeContour::Execute()
{
  vtkPolyData *output = this->GetOutput();
  int *ext;

  this->Reader->Update();
  ext = this->Reader->GetOutput()->GetWholeExtent();
  if (this->XSlice < ext[0])
    {
    this->XSlice = ext[0];
    }
  if (this->XSlice > ext[1])
    {
    this->XSlice = ext[1];
    }
  if (this->YSlice < ext[2])
    {
    this->YSlice = ext[2];
    }
  if (this->YSlice > ext[3])
    {
    this->YSlice = ext[3];
    }
  if (this->ZSlice < ext[4])
    {
    this->ZSlice = ext[4];
    }
  if (this->ZSlice > ext[5])
    {
    this->ZSlice = ext[5];
    }
  this->XClip->SetOutputWholeExtent(this->XSlice, this->XSlice,
                                    ext[2], ext[3], ext[4], ext[5]);
  this->XClip->Update();
  this->GetXSliceOutput()->ShallowCopy(this->XClip->GetOutput());
  this->YClip->SetOutputWholeExtent(ext[0], ext[1], this->YSlice, 
                                    this->YSlice, ext[4], ext[5]);
  this->YClip->Update();
  this->GetYSliceOutput()->ShallowCopy(this->YClip->GetOutput());
  this->ZClip->SetOutputWholeExtent(ext[0], ext[1], ext[2], ext[3],
                                    this->ZSlice, this->ZSlice);
  this->ZClip->Update();
  this->GetZSliceOutput()->ShallowCopy(this->ZClip->GetOutput());

  this->Contour->Update();
  output->ShallowCopy(this->Contour->GetOutput());
}

//----------------------------------------------------------------------------
void vtkRunTimeContour::UpdateWidgets()
{
  int *ext;

  this->Reader->Modified();
  this->Reader->Update();
  ext = this->Reader->GetOutput()->GetWholeExtent();

  if (this->XScale)
    {
    this->XScale->SetRange(ext[0], ext[1]);
    }
  if (this->YScale)
    {
    this->YScale->SetRange(ext[2], ext[3]);
    }
  if (this->ZScale)
    {
    this->ZScale->SetRange(ext[4], ext[5]);
    }

  this->Reader->GetOutput()->GetScalarRange(this->Range);
  if (this->ContourScale->GetValue() < this->Range[0])
    {
    this->ContourScale->SetRange(this->Range[0], this->Range[1]);
    this->SetContourValue((this->Range[1] - this->Range[0])/2.0);
    }
}

//----------------------------------------------------------------------------
void vtkRunTimeContour::SetContourValue(float value)
{
  this->Contour->SetFirstValue(value);
  this->ContourScale->SetValue(value);
  this->Modified();
}

//----------------------------------------------------------------------------
float vtkRunTimeContour::GetContourValue()
{
  return this->Contour->GetFirstValue();
}

//----------------------------------------------------------------------------
void vtkRunTimeContour::SetFileName(char *filename)
{
  this->Reader->SetFileName(filename);
  this->Modified();
}

//----------------------------------------------------------------------------
char *vtkRunTimeContour::GetFileName()
{
  return this->Reader->GetFileName();
}
