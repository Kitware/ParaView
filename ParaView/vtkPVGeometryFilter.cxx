/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVGeometryFilter.cxx
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
#include "vtkPVGeometryFilter.h"
#include "vtkImageData.h"
#include "vtkOutlineSource.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"


//------------------------------------------------------------------------------
vtkPVGeometryFilter* vtkPVGeometryFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVGeometryFilter");
  if(ret)
    {
    return (vtkPVGeometryFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVGeometryFilter;
}




//----------------------------------------------------------------------------
vtkPVGeometryFilter::vtkPVGeometryFilter ()
{
  this->Mode = VTK_PV_SURFACE;
}

//----------------------------------------------------------------------------
vtkPVGeometryFilter::~vtkPVGeometryFilter ()
{
}


//----------------------------------------------------------------------------
void vtkPVGeometryFilter::ComputeInputUpdateExtents( vtkDataObject *output)
{
  vtkDataSet *input = this->GetInput();
  vtkImageData *image = vtkImageData::SafeDownCast(input);
  
  // Force update of volumes for timing.
  if (0 && this->Mode == VTK_PV_IMAGE_OUTLINE && image)
    {
    image->SetUpdateExtent(0, -1, 0, -1, 0, -1);
    }
  else
    {
    this->vtkDataSetSurfaceFilter::ComputeInputUpdateExtents(output);
    }
}



//----------------------------------------------------------------------------
void vtkPVGeometryFilter::UpdateData(vtkDataObject *output)
{
  vtkDataSet *input = this->GetInput();
  vtkImageData *image = vtkImageData::SafeDownCast(input);
  int idx;

  // Lets update even on images for now.  Easy timing.
  if (1 || image == NULL || this->Mode != VTK_PV_IMAGE_OUTLINE)
    {
    this->vtkDataSetSurfaceFilter::UpdateData(output);
    return;
    }  
  
  // Initialize all the outputs
  for (idx = 0; idx < this->NumberOfOutputs; idx++)
    {
    if (this->Outputs[idx])
      {
      this->Outputs[idx]->PrepareForNewData(); 
      }
    }
 
  // If there is a start method, call it
//  if ( this->StartMethod )
//    {
//    (*this->StartMethod)(this->StartMethodArg);
//    }
  
  if ( this->StartTag )
    {
    // All the examples of using this I see have NULL as the 2nd parameter.
    this->InvokeEvent(vtkCommand::StartEvent, NULL);
    }

  // Execute this object - we have not aborted yet, and our progress
  // before we start to execute is 0.0.
  this->AbortExecute = 0;
  this->Progress = 0.0;
  if (this->NumberOfInputs < this->NumberOfRequiredInputs)
    {
    vtkErrorMacro(<< "At least " << this->NumberOfRequiredInputs << " inputs are required but only " << this->NumberOfInputs << " are specified");
    }
  else
    {
    this->Execute();
    }

  // If we ended due to aborting, push the progress up to 1.0 (since
  // it probably didn't end there)
  if ( !this->AbortExecute )
    {
    this->UpdateProgress(1.0);
    }

  // Call the end method, if there is one
//  if ( this->EndMethod )
//    {
//    (*this->EndMethod)(this->EndMethodArg);
//    }
  
  if ( this->EndTag )
    {
    this->InvokeEvent(vtkCommand::EndEvent, NULL);
    }
    
  // Now we have to mark the data as up to data.
  for (idx = 0; idx < this->NumberOfOutputs; ++idx)
    {
    if (this->Outputs[idx])
      {
      this->Outputs[idx]->DataHasBeenGenerated();
      }
    }
  
  // Information gets invalidated as soon as Update is called,
  // so validate it again here.
  this->InformationTime.Modified();
}




//----------------------------------------------------------------------------
void vtkPVGeometryFilter::Execute()
{
  float *spacing;
  float *origin;
  int *ext;
  float bounds[6];
  vtkPolyData *output = this->GetOutput();
  vtkDataSet *ds = this->GetInput();
  vtkImageData *input = vtkImageData::SafeDownCast(ds);

  if (input == NULL || this->Mode != VTK_PV_IMAGE_OUTLINE)
    {
    this->vtkDataSetSurfaceFilter::Execute();
    return;
    }  
  
  //
  // Let OutlineSource do all the work
  //
  
  if (output->GetUpdatePiece() == 0)
    {
    spacing = input->GetSpacing();
    origin = input->GetOrigin();
    ext = input->GetWholeExtent();
    
    bounds[0] = spacing[0] * ((float)ext[0]) + origin[0];
    bounds[1] = spacing[0] * ((float)ext[1]) + origin[0];
    bounds[2] = spacing[1] * ((float)ext[2]) + origin[1];
    bounds[3] = spacing[1] * ((float)ext[3]) + origin[1];
    bounds[4] = spacing[2] * ((float)ext[4]) + origin[2];
    bounds[5] = spacing[2] * ((float)ext[5]) + origin[2];

    vtkOutlineSource *outline = vtkOutlineSource::New();
    outline->SetBounds(bounds);
    outline->Update();

    output->SetPoints(outline->GetOutput()->GetPoints());
    output->SetLines(outline->GetOutput()->GetLines());
    outline->Delete();
    }

}


//----------------------------------------------------------------------------
void vtkPVGeometryFilter::ExecuteInformation()
{
  vtkDataSet *ds = this->GetInput();
  vtkImageData *input = vtkImageData::SafeDownCast(ds);

  if (input == NULL || this->Mode != VTK_PV_IMAGE_OUTLINE)
    {
    this->vtkDataSetSurfaceFilter::ExecuteInformation();
    return;
    }  
  
  vtkDebugMacro(<< "Creating dataset outline");

  //
  // Let OutlineSource do all the work
  //
  this->vtkSource::ExecuteInformation();
}
