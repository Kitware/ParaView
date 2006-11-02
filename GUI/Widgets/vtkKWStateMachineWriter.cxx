/*=========================================================================

  Module:    vtkKWStateMachineWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWStateMachineWriter.h"

#include "vtkKWStateMachine.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkKWStateMachineWriter, "1.1");

vtkCxxSetObjectMacro(vtkKWStateMachineWriter,Input,vtkKWStateMachine);

//----------------------------------------------------------------------------
vtkKWStateMachineWriter::vtkKWStateMachineWriter()
{
  this->Input = NULL;
  this->WriteSelfLoop = 1;
}

//----------------------------------------------------------------------------
vtkKWStateMachineWriter::~vtkKWStateMachineWriter()
{
  this->SetInput(NULL);
}

//----------------------------------------------------------------------------
void vtkKWStateMachineWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "WriteSelfLoop: " 
     << (this->WriteSelfLoop ? "On" : "Off") << endl;
  
  os << indent << "Input: ";
  if (this->Input)
    {
    os << endl;
    this->Input->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
}
