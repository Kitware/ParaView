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
#include "vtkPVXMLElement.h"
#include "vtkSMCommunicationModule.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMDoubleVectorProperty);
vtkCxxRevisionMacro(vtkSMDoubleVectorProperty, "1.6");

struct vtkSMDoubleVectorPropertyInternals
{
  vtkstd::vector<double> Values;
};

//---------------------------------------------------------------------------
vtkSMDoubleVectorProperty::vtkSMDoubleVectorProperty()
{
  this->Internals = new vtkSMDoubleVectorPropertyInternals;
  this->ArgumentIsArray = 0;
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
    if (this->ArgumentIsArray)
      {
      *str << vtkClientServerStream::InsertArray(
        &(this->Internals->Values[0]), numArgs);
      }
    else
      {
      for(int i=0; i<numArgs; i++)
        {
        *str << this->GetElement(i);
        }
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
      if (this->UseIndex)
        {
        *str << i;
        }
      if (this->ArgumentIsArray)
        {
        *str << vtkClientServerStream::InsertArray(
          &(this->Internals->Values[i*this->NumberOfElementsPerCommand]),
          this->NumberOfElementsPerCommand);
        }
      else
        {
        for (int j=0; j<this->NumberOfElementsPerCommand; j++)
          {
          *str << this->GetElement(i*this->NumberOfElementsPerCommand+j);
          }
        }
      *str << vtkClientServerStream::End;
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::SetNumberOfElements(unsigned int num)
{
  this->Internals->Values.resize(num);
  this->Modified();
}

//---------------------------------------------------------------------------
unsigned int vtkSMDoubleVectorProperty::GetNumberOfElements()
{
  return this->Internals->Values.size();
}

//---------------------------------------------------------------------------
double vtkSMDoubleVectorProperty::GetElement(unsigned int idx)
{
  return this->Internals->Values[idx];
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::SetElement(unsigned int idx, double value)
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
void vtkSMDoubleVectorProperty::SetElements(const double* values)
{
  int numArgs = this->GetNumberOfElements();
  memcpy(&this->Internals->Values[0], values, numArgs*sizeof(double));
  this->Modified();
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::ReadXMLAttributes(vtkPVXMLElement* element)
{
  int retVal;

  retVal = this->Superclass::ReadXMLAttributes(element);
  if (!retVal)
    {
    return retVal;
    }

  int arg_is_array;
  retVal = element->GetScalarAttribute("argument_is_array", &arg_is_array);
  if(retVal) 
    { 
    this->SetArgumentIsArray(arg_is_array); 
    }

  int numElems = this->GetNumberOfElements();
  if (numElems > 0)
    {
    double* initVal = new double[numElems];
    int numRead = element->GetVectorAttribute("default_values",
                                              numElems,
                                              initVal);

    if (numRead > 0)
      {
      if (numRead != numElems)
        {
        vtkErrorMacro("The number of default values does not match the number "
                      "of elements. Initialization failed.");
        delete[] initVal;
        return 0;
        }
      this->SetElements(initVal);
      }
    delete[] initVal;
    }
    
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::SaveState(
  const char* name, ofstream* file, vtkIndent indent)
{
  *file << indent << name << " : ";
  unsigned int size = this->GetNumberOfElements();
  for (unsigned int i=0; i<size; i++)
    {
    *file << this->GetElement(i) << " ";
    }
  *file << endl;
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ArgumentIsArray: " << this->ArgumentIsArray << endl;
}
