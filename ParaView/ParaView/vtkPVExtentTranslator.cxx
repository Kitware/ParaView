/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtentTranslator.cxx
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
#include "vtkPVExtentTranslator.h"

#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSource.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVExtentTranslator);
vtkCxxRevisionMacro(vtkPVExtentTranslator, "1.14");

//vtkCxxSetObjectMacro(vtkPVExtentTranslator, OriginalSource, vtkDataSet);

//-----------------------------------------------------------------------------
vtkPVExtentTranslator::vtkPVExtentTranslator()
{
  this->OriginalSource = NULL;
}

//-----------------------------------------------------------------------------
vtkPVExtentTranslator::~vtkPVExtentTranslator()
{
  this->SetOriginalSource(NULL);
}

//-----------------------------------------------------------------------------
void vtkPVExtentTranslator::SetOriginalSource(vtkDataSet *d)
{
  // No reference counting because of nasty loop. (TransmitPolyData ...)
  // Propagation should make it safe to ignore reference counting.
  this->OriginalSource = d;
} 


//-----------------------------------------------------------------------------
int vtkPVExtentTranslator::PieceToExtentThreadSafe(int piece, int numPieces, 
                                      int ghostLevel, int *wholeExtent, 
                                      int *resultExtent, int splitMode, 
                                      int byPoints)
{
  int ret;
  int origWholeExt[6];

  if (this->OriginalSource == NULL)
    {
    memcpy(origWholeExt, wholeExtent, sizeof(int)*6);    
    }
  else
    {
    this->OriginalSource->UpdateInformation();
    this->OriginalSource->GetWholeExtent(origWholeExt);
    }

  memcpy(resultExtent, origWholeExt, sizeof(int)*6);
  if (byPoints)
    {
    ret = this->SplitExtentByPoints(piece, numPieces, resultExtent, splitMode);
    }
  else
    {
    ret = this->SplitExtent(piece, numPieces, resultExtent, splitMode);
    }
    
  if (ret == 0)
    {
    // Nothing in this piece.
    resultExtent[0] = resultExtent[2] = resultExtent[4] = 0;
    resultExtent[1] = resultExtent[3] = resultExtent[5] = -1;
    return 0;
    }

  if (ghostLevel > 0)
    {
    resultExtent[0] -= ghostLevel;
    resultExtent[1] += ghostLevel;
    resultExtent[2] -= ghostLevel;
    resultExtent[3] += ghostLevel;
    resultExtent[4] -= ghostLevel;
    resultExtent[5] += ghostLevel;
    }
    
  if (resultExtent[0] < wholeExtent[0])
    {
    resultExtent[0] = wholeExtent[0];
    }
  if (resultExtent[1] > wholeExtent[1])
    {
    resultExtent[1] = wholeExtent[1];
    }
  if (resultExtent[2] < wholeExtent[2])
    {
    resultExtent[2] = wholeExtent[2];
    }
  if (resultExtent[3] > wholeExtent[3])
    {
    resultExtent[3] = wholeExtent[3];
    }
  if (resultExtent[4] < wholeExtent[4])
    {
    resultExtent[4] = wholeExtent[4];
    }
  if (resultExtent[5] > wholeExtent[5])
    {
    resultExtent[5] = wholeExtent[5];
    }

  if (resultExtent[0] > resultExtent[1] ||
      resultExtent[2] > resultExtent[3] ||
      resultExtent[4] > resultExtent[5])
    {
    // Nothing in this piece.
    resultExtent[0] = resultExtent[2] = resultExtent[4] = 0;
    resultExtent[1] = resultExtent[3] = resultExtent[5] = -1;
    return 0;
    }
    
  return 1;
}



//-----------------------------------------------------------------------------
void vtkPVExtentTranslator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Original Source: (" << this->OriginalSource << ")\n";
}







