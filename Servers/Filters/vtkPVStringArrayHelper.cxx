/*=========================================================================

  Program:   ParaView
  Module:    vtkPVStringArrayHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVStringArrayHelper.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkStringArray.h"

#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVStringArrayHelper);
vtkCxxRevisionMacro(vtkPVStringArrayHelper, "1.1");

//----------------------------------------------------------------------------
class vtkPVStringArrayHelperInternals
{
public:
  vtkClientServerStream Result;
};

//----------------------------------------------------------------------------
vtkPVStringArrayHelper::vtkPVStringArrayHelper()
{
  this->Internal = new vtkPVStringArrayHelperInternals;
}

//----------------------------------------------------------------------------
vtkPVStringArrayHelper::~vtkPVStringArrayHelper()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPVStringArrayHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
const vtkClientServerStream&
vtkPVStringArrayHelper::GetStringList(vtkStringArray* strings)
{
  // Reset the stream for a new list of array names.
  this->Internal->Result.Reset();
  this->Internal->Result << vtkClientServerStream::Reply;

  // Make sure we have a process module.
  if(this->ProcessModule && strings)
    {
    // Pack the contents of the string into a stream.

    // Get the local process interpreter.
    vtkClientServerInterpreter* interp =
      this->ProcessModule->GetInterpreter();

    vtkIdType numStrings = strings->GetNumberOfValues();
    for (vtkIdType i=0; i<numStrings; i++)
      {
      this->Internal->Result << strings->GetValue(i);
      }
    }
  // End the message and return the stream.
  this->Internal->Result << vtkClientServerStream::End;
  return this->Internal->Result;
}
