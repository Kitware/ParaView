/*=========================================================================

  Program:   ParaView
  Module:    vtkPVNumberOfOutputsInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVNumberOfOutputsInformation.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkSource.h"

vtkStandardNewMacro(vtkPVNumberOfOutputsInformation);
vtkCxxRevisionMacro(vtkPVNumberOfOutputsInformation, "1.2");

//----------------------------------------------------------------------------
vtkPVNumberOfOutputsInformation::vtkPVNumberOfOutputsInformation()
{
  this->RootOnly = 1;
  this->NumberOfOutputs = 0;
}

//----------------------------------------------------------------------------
vtkPVNumberOfOutputsInformation::~vtkPVNumberOfOutputsInformation()
{
}

//----------------------------------------------------------------------------
void vtkPVNumberOfOutputsInformation::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfOutputs: " << this->NumberOfOutputs << "\n";
}

//----------------------------------------------------------------------------
void vtkPVNumberOfOutputsInformation::CopyFromObject(vtkObject* obj)
{
  this->NumberOfOutputs = 0;
  vtkAlgorithm* algorithm = vtkAlgorithm::SafeDownCast(obj);
  vtkSource* source = vtkSource::SafeDownCast(obj);
  if(!algorithm)
    {
    vtkErrorMacro("Could not downcast vtkAlgorithm.");
    return;
    }
  if(source)
    {
    this->NumberOfOutputs = source->GetNumberOfOutputs();
    }
  else
    {
    this->NumberOfOutputs = algorithm->GetNumberOfOutputPorts();
    }
}

//----------------------------------------------------------------------------
void vtkPVNumberOfOutputsInformation::AddInformation(vtkPVInformation*)
{
}

//----------------------------------------------------------------------------
void
vtkPVNumberOfOutputsInformation::CopyToStream(vtkClientServerStream* css) const
{
  css->Reset();
  *css << vtkClientServerStream::Reply << this->NumberOfOutputs
       << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void
vtkPVNumberOfOutputsInformation
::CopyFromStream(const vtkClientServerStream* css)
{
  css->GetArgument(0, 0, &this->NumberOfOutputs);
}
