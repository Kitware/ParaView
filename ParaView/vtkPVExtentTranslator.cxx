/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVExtentTranslator.cxx
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

#include "vtkPVExtentTranslator.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkPVExtentTranslator* vtkPVExtentTranslator::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVExtentTranslator");
  if(ret)
    {
    return (vtkPVExtentTranslator*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVExtentTranslator;
}


//----------------------------------------------------------------------------
vtkPVExtentTranslator::vtkPVExtentTranslator()
{
  this->OriginalSource = NULL;
}

//----------------------------------------------------------------------------
vtkPVExtentTranslator::~vtkPVExtentTranslator()
{
  this->SetOriginalSource(NULL);
}

//----------------------------------------------------------------------------
void vtkPVExtentTranslator::PieceToExtent()
{
  if (this->OriginalSource == NULL)
    { // If the user has not set the original source, then just default
    // to the method in the superclass.
    this->vtkExtentTranslator::PieceToExtent();
    return;
    }

  this->OriginalSource->UpdateInformation();
  this->OriginalSource->GetWholeExtent(this->Extent);
  if (this->SplitExtent(this->Piece, this->NumberOfPieces, this->Extent) == 0)
    {
    // Nothing in this piece.
    this->Extent[0] = this->Extent[2] = this->Extent[4] = 0;
    this->Extent[1] = this->Extent[3] = this->Extent[5] = -1;
    return;
    }

  // Clip with the whole extent passed in.
  if (this->Extent[0] < this->WholeExtent[0])
    {
    this->Extent[0] = this->WholeExtent[0];
    }  
  if (this->Extent[1] > this->WholeExtent[1])
    {
    this->Extent[1] = this->WholeExtent[1];
    }
  if (this->Extent[2] < this->WholeExtent[2])
    {
    this->Extent[2] = this->WholeExtent[2];
    }  
  if (this->Extent[3] > this->WholeExtent[3])
    {
    this->Extent[3] = this->WholeExtent[3];
    }
  if (this->Extent[4] < this->WholeExtent[4])
    {
    this->Extent[4] = this->WholeExtent[4];
    }  
  if (this->Extent[5] > this->WholeExtent[5])
    {
    this->Extent[5] = this->WholeExtent[5];
    }
}



//----------------------------------------------------------------------------
void vtkPVExtentTranslator::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Original Source: (" << this->OriginalSource << ")\n";
}







