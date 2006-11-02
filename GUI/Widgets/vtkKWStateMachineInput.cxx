/*=========================================================================

  Module:    vtkKWStateMachineInput.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWStateMachineInput.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWStateMachineInput);
vtkCxxRevisionMacro(vtkKWStateMachineInput, "1.1");

vtkIdType vtkKWStateMachineInput::IdCounter = 1;

//----------------------------------------------------------------------------
vtkKWStateMachineInput::vtkKWStateMachineInput()
{
  this->Id = vtkKWStateMachineInput::IdCounter++;
  this->Name = NULL;
}

//----------------------------------------------------------------------------
vtkKWStateMachineInput::~vtkKWStateMachineInput()
{
  this->SetName(NULL);
}

//----------------------------------------------------------------------------
void vtkKWStateMachineInput::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Id: " << this->Id << endl;
  os << indent << "Name: " << (this->Name ? this->Name : "None") << endl;
}
