/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIntVectorProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMIntVectorProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkSMCommunicationModule.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMIntVectorProperty);
vtkCxxRevisionMacro(vtkSMIntVectorProperty, "1.1");

struct vtkSMIntVectorPropertyInternals
{
  vtkstd::vector<int> Values;
};

//---------------------------------------------------------------------------
vtkSMIntVectorProperty::vtkSMIntVectorProperty()
{
  this->Internals = new vtkSMIntVectorPropertyInternals;
}

//---------------------------------------------------------------------------
vtkSMIntVectorProperty::~vtkSMIntVectorProperty()
{
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::AppendCommandToStream(
    vtkClientServerStream* str, vtkClientServerID objectId )
{
  if (!this->Command)
    {
    return;
    }

  if (!this->RepeatCommand)
    {
    *str << vtkClientServerStream::Invoke << objectId << this->Command;
    int numArgs = this->GetNumberOfElements();
    for(int i=0; i<numArgs; i++)
      {
      *str << this->GetElement(i);
      }
    *str << vtkClientServerStream::End;
    }
  else
    {
    int numArgs = this->GetNumberOfElements();
    int numCommands = numArgs / this->NumberOfElementsPerCommand;
    for(int i=0; i<numCommands; i++)
      {
      *str << vtkClientServerStream::Invoke << objectId << this->Command;
      for (int j=0; j<this->NumberOfElementsPerCommand; j++)
        {
        if (this->UseIndex)
          {
          *str << i;
          }
        *str << this->GetElement(i*this->NumberOfElementsPerCommand+j);
        }
      *str << vtkClientServerStream::End;
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::SetNumberOfElements(int num)
{
  this->Internals->Values.resize(num);
  this->Modified();
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::GetNumberOfElements()
{
  return this->Internals->Values.size();
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::GetElement(int idx)
{
  return this->Internals->Values[idx];
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::SetElement(int idx, int value)
{
  if (idx >= this->GetNumberOfElements())
    {
    this->SetNumberOfElements(idx+1);
    }
  this->Internals->Values[idx] = value;
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::SetElements1(int value0)
{
  this->SetElement(0, value0);
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::SetElements2(int value0, int value1)
{
  this->SetElement(0, value0);
  this->SetElement(1, value1);
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::SetElements3(int value0, int value1, int value2)
{
  this->SetElement(0, value0);
  this->SetElement(1, value1);
  this->SetElement(2, value2);
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::SetElements(int* values)
{
  int numArgs = this->GetNumberOfElements();
  memcpy(&this->Internals->Values[0], values, numArgs*sizeof(int));
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
