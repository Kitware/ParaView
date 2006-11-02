/*=========================================================================

  Module:    vtkKWStateMachineState.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWStateMachineState.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWStateMachineState);
vtkCxxRevisionMacro(vtkKWStateMachineState, "1.1");

vtkIdType vtkKWStateMachineState::IdCounter = 1;

//----------------------------------------------------------------------------
vtkKWStateMachineState::vtkKWStateMachineState()
{
  this->Id = vtkKWStateMachineState::IdCounter++;
  this->Name = NULL;
  this->Description = NULL;
}

//----------------------------------------------------------------------------
vtkKWStateMachineState::~vtkKWStateMachineState()
{
  this->SetName(NULL);
  this->SetDescription(NULL);
}

//----------------------------------------------------------------------------
void vtkKWStateMachineState::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Id: " << this->Id << endl;
  os << indent << "Name: " << (this->Name ? this->Name : "None") << endl;
  os << indent << "Description: " 
     << (this->Description ? this->Description : "None") << endl;
}
