/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDoubleVectorProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDoubleVectorProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkSMCommunicationModule.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMDoubleVectorProperty);
vtkCxxRevisionMacro(vtkSMDoubleVectorProperty, "1.1");

struct vtkSMDoubleVectorPropertyInternals
{
  vtkstd::vector<double> Values;
};

//---------------------------------------------------------------------------
vtkSMDoubleVectorProperty::vtkSMDoubleVectorProperty()
{
  this->Internals = new vtkSMDoubleVectorPropertyInternals;
}

//---------------------------------------------------------------------------
vtkSMDoubleVectorProperty::~vtkSMDoubleVectorProperty()
{
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::AppendCommandToStream(
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
void vtkSMDoubleVectorProperty::SetNumberOfElements(int num)
{
  this->Internals->Values.resize(num);
  this->Modified();
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::GetNumberOfElements()
{
  return this->Internals->Values.size();
}

//---------------------------------------------------------------------------
double vtkSMDoubleVectorProperty::GetElement(int idx)
{
  return this->Internals->Values[idx];
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::SetElement(int idx, double value)
{
  if (idx >= this->GetNumberOfElements())
    {
    this->SetNumberOfElements(idx+1);
    }
  this->Internals->Values[idx] = value;
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::SetElements1(double value0)
{
  this->SetElement(0, value0);
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::SetElements2(double value0, double value1)
{
  this->SetElement(0, value0);
  this->SetElement(1, value1);
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::SetElements3(
  double value0, double value1, double value2)
{
  this->SetElement(0, value0);
  this->SetElement(1, value1);
  this->SetElement(2, value2);
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::SetElements(double* values)
{
  int numArgs = this->GetNumberOfElements();
  memcpy(&this->Internals->Values[0], values, numArgs*sizeof(double));
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
