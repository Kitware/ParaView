/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerSelectTimeSet.cxx
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
#include "vtkPVServerSelectTimeSet.h"

#include "vtkClientServerInterpreter.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkGenericEnSightReader.h"
#include "vtkDataArrayCollection.h"
#include "vtkDataArrayCollectionIterator.h"

#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVServerSelectTimeSet);
vtkCxxRevisionMacro(vtkPVServerSelectTimeSet, "1.2");

//----------------------------------------------------------------------------
class vtkPVServerSelectTimeSetInternals
{
public:
  vtkClientServerStream Result;
};

//----------------------------------------------------------------------------
vtkPVServerSelectTimeSet::vtkPVServerSelectTimeSet()
{
  this->Internal = new vtkPVServerSelectTimeSetInternals;
}

//----------------------------------------------------------------------------
vtkPVServerSelectTimeSet::~vtkPVServerSelectTimeSet()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPVServerSelectTimeSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
const vtkClientServerStream&
vtkPVServerSelectTimeSet::GetTimeSets(vtkGenericEnSightReader* reader)
{
  // Reset the stream for a new list of time sets.
  this->Internal->Result.Reset();

  // Get the time sets from the reader.
  vtkDataArrayCollection* timeSets = reader->GetTimeSets();

  // Iterate through the time sets.
  vtkDataArrayCollectionIterator* iter = vtkDataArrayCollectionIterator::New();
  iter->SetCollection(timeSets);
  for(iter->GoToFirstItem(); !iter->IsDoneWithTraversal();
      iter->GoToNextItem())
    {
    // Each time set is stored in one message.
    this->Internal->Result << vtkClientServerStream::Reply;
    vtkDataArray* da = iter->GetDataArray();
    for(int i=0; i < da->GetNumberOfTuples(); ++i)
      {
      this->Internal->Result << da->GetTuple1(i);
      }
    this->Internal->Result << vtkClientServerStream::End;
    }
  iter->Delete();

  // Return the stream.
  return this->Internal->Result;
}
