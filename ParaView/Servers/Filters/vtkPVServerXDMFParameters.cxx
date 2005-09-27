/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerXDMFParameters.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVServerXDMFParameters.h"

#include "vtkClientServerInterpreter.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkXdmfReader.h"
#include "vtkClientServerStream.h"

#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVServerXDMFParameters);
vtkCxxRevisionMacro(vtkPVServerXDMFParameters, "1.1");

//----------------------------------------------------------------------------
class vtkPVServerXDMFParametersInternals
{
public:
  vtkClientServerStream Result;
};

//----------------------------------------------------------------------------
vtkPVServerXDMFParameters::vtkPVServerXDMFParameters()
{
  this->Internal = new vtkPVServerXDMFParametersInternals;
}

//----------------------------------------------------------------------------
vtkPVServerXDMFParameters::~vtkPVServerXDMFParameters()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPVServerXDMFParameters::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
const vtkClientServerStream&
vtkPVServerXDMFParameters::GetParameters(vtkXdmfReader* reader)
{
  // Reset the result stream for a new set of parameters.
  this->Internal->Result.Reset();
  this->Internal->Result << vtkClientServerStream::Reply;

  // Store each parameter name, index, and range in the stream.
  for(int i=0; i < reader->GetNumberOfParameters(); ++i)
    {
    int range[3];
    reader->GetParameterRange(i, range);
    this->Internal->Result << reader->GetParameterName(i)
                          << reader->GetParameterIndex(i)
                          << vtkClientServerStream::InsertArray(range, 3);
    }

  // Finish the message and return the result stream.
  this->Internal->Result << vtkClientServerStream::End;
  return this->Internal->Result;
}
