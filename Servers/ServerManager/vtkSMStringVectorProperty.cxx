/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStringVectorProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStringVectorProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMCommunicationModule.h"

#include <vtkstd/vector>
#include "vtkStdString.h"

vtkStandardNewMacro(vtkSMStringVectorProperty);
vtkCxxRevisionMacro(vtkSMStringVectorProperty, "1.2");

struct vtkSMStringVectorPropertyInternals
{
  vtkstd::vector<vtkStdString> Values;
  vtkstd::vector<int> ElementTypes;
};

//---------------------------------------------------------------------------
vtkSMStringVectorProperty::vtkSMStringVectorProperty()
{
  this->Internals = new vtkSMStringVectorPropertyInternals;
}

//---------------------------------------------------------------------------
vtkSMStringVectorProperty::~vtkSMStringVectorProperty()
{
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::SetElementType(unsigned int idx, int type)
{
  unsigned int size = this->Internals->ElementTypes.size();
  if (idx >= size)
    {
    this->Internals->ElementTypes.resize(idx+1);
    }
  for (unsigned int i=size; i<=idx; i++)
    {
    this->Internals->ElementTypes[i] = STRING;
    }
  this->Internals->ElementTypes[idx] = type;
}

//---------------------------------------------------------------------------
int vtkSMStringVectorProperty::GetElementType(unsigned int idx)
{
  if (idx >= this->Internals->ElementTypes.size())
    {
    return STRING;
    }
  return this->Internals->ElementTypes[idx];
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::AppendCommandToStream(
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
      // Convert to the appropriate type and add to stream
      switch (this->GetElementType(0))
        {
        case INT:
          *str << atoi(this->GetElement(i));
          break;
        case DOUBLE:
          *str << atof(this->GetElement(i));
          break;
        case STRING:
          *str << this->GetElement(i);
          break;
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
      for (int j=0; j<this->NumberOfElementsPerCommand; j++)
        {
        // Convert to the appropriate type and add to stream
        switch (this->GetElementType(j))
          {
          case INT:
            *str << atoi(this->GetElement(i*this->NumberOfElementsPerCommand+j));
            break;
          case DOUBLE:
            *str << atof(this->GetElement(i*this->NumberOfElementsPerCommand+j));
            break;
          case STRING:
            *str << this->GetElement(i*this->NumberOfElementsPerCommand+j);
            break;
          }
        }
      *str << vtkClientServerStream::End;
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::SetNumberOfElements(unsigned int num)
{
  this->Internals->Values.resize(num);
  this->Modified();
}

//---------------------------------------------------------------------------
unsigned int vtkSMStringVectorProperty::GetNumberOfElements()
{
  return this->Internals->Values.size();
}

//---------------------------------------------------------------------------
const char* vtkSMStringVectorProperty::GetElement(unsigned int idx)
{
  return this->Internals->Values[idx].c_str();
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::SetElement(unsigned int idx, const char* value)
{
  if (idx >= this->GetNumberOfElements())
    {
    this->SetNumberOfElements(idx+1);
    }
  this->Internals->Values[idx] = value;
  this->Modified();
}

//---------------------------------------------------------------------------
int vtkSMStringVectorProperty::ReadXMLAttributes(vtkPVXMLElement* element)
{
  int retVal;

  retVal = this->Superclass::ReadXMLAttributes(element);
  if (!retVal)
    {
    return retVal;
    }

  int numEls = this->GetNumberOfElementsPerCommand();
  int* eTypes = new int[numEls];
  int numElsRead = element->GetVectorAttribute("element_types", numEls, eTypes);
  for (int i=0; i<numElsRead; i++)
    {
    this->Internals->ElementTypes.push_back(eTypes[i]);
    }
  delete[] eTypes;

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
