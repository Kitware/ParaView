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
#include "vtkProcessModule.h"

#include <vtkstd/vector>
#include "vtkStdString.h"

vtkStandardNewMacro(vtkSMStringVectorProperty);
vtkCxxRevisionMacro(vtkSMStringVectorProperty, "1.17.8.1");

struct vtkSMStringVectorPropertyInternals
{
  vtkstd::vector<vtkStdString> Values;
  vtkstd::vector<vtkStdString> UncheckedValues;
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
  vtkSMProxy*, vtkClientServerStream* str, vtkClientServerID objectId )
{
  if (!this->Command || this->InformationOnly)
    {
    return;
    }

  if (this->CleanCommand)
    {
    *str << vtkClientServerStream::Invoke
      << objectId << this->CleanCommand
      << vtkClientServerStream::End;
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
void vtkSMStringVectorProperty::SetNumberOfUncheckedElements(unsigned int num)
{
  this->Internals->UncheckedValues.resize(num);
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::SetNumberOfElements(unsigned int num)
{
  this->Internals->Values.resize(num);
  this->Internals->UncheckedValues.resize(num);
  this->Modified();
}

//---------------------------------------------------------------------------
unsigned int vtkSMStringVectorProperty::GetNumberOfUncheckedElements()
{
  return this->Internals->UncheckedValues.size();
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
const char* vtkSMStringVectorProperty::GetUncheckedElement(unsigned int idx)
{
  return this->Internals->UncheckedValues[idx].c_str();
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::SetUncheckedElement(
  unsigned int idx, const char* value)
{
  if (!value)
    {
    value = "";
    }

  if (idx >= this->GetNumberOfUncheckedElements())
    {
    this->SetNumberOfUncheckedElements(idx+1);
    }
  this->Internals->UncheckedValues[idx] = value;
}

//---------------------------------------------------------------------------
int vtkSMStringVectorProperty::SetElement(unsigned int idx, const char* value)
{
  if (!value)
    {
    value = "";
    }

  if ( vtkSMProperty::GetCheckDomains() )
    {
    for(unsigned int i=0; i<this->GetNumberOfElements(); i++)
      {
      this->SetUncheckedElement(i, this->GetElement(i));
      }
    
    this->SetUncheckedElement(idx, value);
    if (!this->IsInDomains())
      {
      this->SetNumberOfUncheckedElements(this->GetNumberOfElements());
      return 0;
      }
    }
  
  if (idx >= this->GetNumberOfElements())
    {
    this->SetNumberOfElements(idx+1);
    }
  this->Internals->Values[idx] = value;
  this->Modified();
  return 1;
}

//---------------------------------------------------------------------------
unsigned int vtkSMStringVectorProperty::GetElementIndex(
  const char *value, int& exists)
{
  unsigned int i;
  for (i = 0; i < this->GetNumberOfElements(); i++)
    {
    if (value && this->Internals->Values[i].c_str() &&
        !strcmp(value, this->Internals->Values[i].c_str()))
      {
      exists = 1;
      return i;
      }
    }
  exists = 0;
  return 0;
}

//---------------------------------------------------------------------------
int vtkSMStringVectorProperty::ReadXMLAttributes(vtkSMProxy* proxy,
                                                 vtkPVXMLElement* element)
{
  int retVal;

  retVal = this->Superclass::ReadXMLAttributes(proxy, element);
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

  int numElems = this->GetNumberOfElements();
  if (numElems > 0)
    {
    const char *initVal = element->GetAttribute("default_values");
    this->SetElement(0, initVal); // what to do with > 1 element?
    }
  
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::SaveState(
  const char* name, ostream* file, vtkIndent indent)
{
  unsigned int size = this->GetNumberOfElements();
  *file << indent << "<Property name=\"" << (this->XMLName?this->XMLName:"")
        << "\" id=\"" << name << "\" ";
  if (size > 0)
    {
    *file << "number_of_elements=\"" << size << "\"";
    }
  *file << ">" << endl;
  for (unsigned int i=0; i<size; i++)
    {
    *file << indent.GetNextIndent() << "<Element index=\""
          << i << "\" " << "value=\"" 
          << (this->GetElement(i)?this->GetElement(i):"") << "\"/>"
          << endl;
    }
  this->Superclass::SaveState(name, file, indent);
  *file << indent << "</Property>" << endl;
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Values: ";
  for (unsigned int i=0; i<this->GetNumberOfElements(); i++)
    {
    os << (this->GetElement(i)?this->GetElement(i):"(nil)") << " ";
    }
  os << endl;
}
