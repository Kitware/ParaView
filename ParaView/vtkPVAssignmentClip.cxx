/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVAssignmentClip.cxx
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
#include "vtkPVAssignmentClip.h"
#include "vtkPlaneSource.h"
#include "vtkObjectFactory.h"
#include "vtkImageClip.h"


//------------------------------------------------------------------------------
vtkPVAssignmentClip* vtkPVAssignmentClip::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVAssignmentClip");
  if(ret)
    {
    return (vtkPVAssignmentClip*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVAssignmentClip;
}




//----------------------------------------------------------------------------
vtkPVAssignmentClip::vtkPVAssignmentClip ()
{
  this->Assignment = NULL;
}

//----------------------------------------------------------------------------
vtkPVAssignmentClip::~vtkPVAssignmentClip ()
{
  this->SetAssignment(NULL);
}

//----------------------------------------------------------------------------
void vtkPVAssignmentClip::ExecuteInformation(vtkImageData *input, 
					     vtkImageData *output)
{
  int *aExt;

  input->GetWholeExtent(this->Extent);

  cerr << "WholeExtent: " << this->Extent[0] << ", " << this->Extent[1] << ", "
       << this->Extent[2] << ", " << this->Extent[3] << ", "
       << this->Extent[4] << ", " << this->Extent[5] << endl;
  
  if (this->Assignment)
    {
    aExt = this->Assignment->GetExtent();
    
    cerr << this->Assignment << " AssignExtent: " << aExt[0] << ", " << aExt[1] << ", "
	 << aExt[2] << ", " << aExt[3] << ", "
	 << aExt[4] << ", " << aExt[5] << endl;    
    
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
  
  cerr << "Extent: " << this->Extent[0] << ", " << this->Extent[1] << ", "
       << this->Extent[2] << ", " << this->Extent[3] << ", "
       << this->Extent[4] << ", " << this->Extent[5] << endl;

  this->SetOutputWholeExtent(this->Extent);

  output->SetWholeExtent(this->Extent);
  output->SetSpacing(output->GetSpacing());
  output->SetOrigin(output->GetOrigin());
  
  
}

