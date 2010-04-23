/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerSelectTimeSet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVServerSelectTimeSet.h"

#include "vtkClientServerInterpreter.h"
#include "vtkObjectFactory.h"
#include "vtkGenericEnSightReader.h"
#include "vtkDataArrayCollection.h"
#include "vtkDataArrayCollectionIterator.h"
#include "vtkClientServerStream.h"

#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVServerSelectTimeSet);

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
