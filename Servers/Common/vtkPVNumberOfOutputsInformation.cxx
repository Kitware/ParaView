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

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkSource.h"

vtkStandardNewMacro(vtkPVNumberOfOutputsInformation);
vtkCxxRevisionMacro(vtkPVNumberOfOutputsInformation, "1.1");

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
  vtkSource* src = vtkSource::SafeDownCast(obj);
  if(!src)
    {
    vtkErrorMacro("Could not downcast vtkSource.");
    return;
    }
  this->NumberOfOutputs = src->GetNumberOfOutputs();
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
