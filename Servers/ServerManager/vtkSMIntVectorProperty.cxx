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
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMIntVectorProperty);
vtkCxxRevisionMacro(vtkSMIntVectorProperty, "1.18");

struct vtkSMIntVectorPropertyInternals
{
  vtkstd::vector<int> Values;
  vtkstd::vector<int> UncheckedValues;
};

//---------------------------------------------------------------------------
vtkSMIntVectorProperty::vtkSMIntVectorProperty()
{
  this->Internals = new vtkSMIntVectorPropertyInternals;
  this->ArgumentIsArray = 0;
}

//---------------------------------------------------------------------------
vtkSMIntVectorProperty::~vtkSMIntVectorProperty()
{
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::AppendCommandToStream(
  vtkSMProxy*, vtkClientServerStream* str, vtkClientServerID objectId )
{
  if (!this->Command || this->InformationOnly)
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
void vtkSMIntVectorProperty::UpdateInformation( 
  int serverIds, vtkClientServerID objectId )
{
  if (!this->InformationOnly)
    {
    return;
    }

  vtkClientServerStream str;
  str << vtkClientServerStream::Invoke 
      << objectId << this->Command
      << vtkClientServerStream::End;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(vtkProcessModule::GetRootId(serverIds), str, 0);

  const vtkClientServerStream& res = 
    pm->GetLastResult(vtkProcessModule::GetRootId(serverIds));

  int numMsgs = res.GetNumberOfMessages();
  if (numMsgs < 1)
    {
    return;
    }

  int numArgs = res.GetNumberOfArguments(0);
  if (numArgs < 1)
    {
    return;
    }

  int argType = res.GetArgumentType(0, 0);

  if (argType == vtkClientServerStream::int32_value ||
      argType == vtkClientServerStream::int16_value ||
      argType == vtkClientServerStream::int8_value)
    {
    int ires;
    int retVal = res.GetArgument(0, 0, &ires);
    if (!retVal)
      {
      vtkErrorMacro("Error getting argument.");
      return;
      }
    this->SetNumberOfElements(1);
    this->SetElement(0, ires);
    }
  else if (argType == vtkClientServerStream::int32_array)
    {
    vtkTypeUInt32 length;
    res.GetArgumentLength(0, 0, &length);
    if (length >= 128)
      {
      vtkErrorMacro("Only arguments of length 128 or less are supported");
      return;
      }
    int values[128];
    int retVal = res.GetArgument(0, 0, values, length);
    if (!retVal)
      {
      vtkErrorMacro("Error getting argument.");
      return;
      }
    this->SetNumberOfElements(length);
    this->SetElements(values);
    }
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::SetNumberOfUncheckedElements(unsigned int num)
{
  this->Internals->UncheckedValues.resize(num);
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::SetNumberOfElements(unsigned int num)
{
  this->Internals->Values.resize(num);
  this->Internals->UncheckedValues.resize(num);
  this->Modified();
}

//---------------------------------------------------------------------------
unsigned int vtkSMIntVectorProperty::GetNumberOfUncheckedElements()
{
  return this->Internals->UncheckedValues.size();
}

//---------------------------------------------------------------------------
unsigned int vtkSMIntVectorProperty::GetNumberOfElements()
{
  return this->Internals->Values.size();
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::GetElement(unsigned int idx)
{
  return this->Internals->Values[idx];
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::GetUncheckedElement(unsigned int idx)
{
  return this->Internals->UncheckedValues[idx];
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::SetUncheckedElement(unsigned int idx, int value)
{
  if (idx >= this->GetNumberOfUncheckedElements())
    {
    this->SetNumberOfUncheckedElements(idx+1);
    }
  this->Internals->UncheckedValues[idx] = value;
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::SetElement(unsigned int idx, int value)
{
  if ( vtkSMProperty::GetCheckDomains() )
    {
    int numArgs = this->GetNumberOfElements();
    memcpy(&this->Internals->UncheckedValues[0], 
           &this->Internals->Values[0], 
           numArgs*sizeof(int));
    
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
int vtkSMIntVectorProperty::SetElements1(int value0)
{
  return this->SetElement(0, value0);
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::SetElements2(int value0, int value1)
{
  int retVal1 = this->SetElement(0, value0);
  int retVal2 = this->SetElement(1, value1);
  return (retVal1 && retVal2);
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::SetElements3(int value0, 
                                          int value1, 
                                          int value2)
{
  int retVal1 = this->SetElement(0, value0);
  int retVal2 = this->SetElement(1, value1);
  int retVal3 = this->SetElement(2, value2);
  return (retVal1 && retVal2 && retVal3);
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::SetElements(const int* values)
{
  int numArgs = this->GetNumberOfElements();

  if ( vtkSMProperty::GetCheckDomains() )
    {
    memcpy(&this->Internals->UncheckedValues[0], 
           values, 
           numArgs*sizeof(int));
    if (!this->IsInDomains())
      {
      return 0;
      }
    }

  memcpy(&this->Internals->Values[0], values, numArgs*sizeof(int));
  this->Modified();
  return 1;
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::ReadXMLAttributes(vtkSMProxy* parent,
                                              vtkPVXMLElement* element)
{
  int retVal;

  retVal = this->Superclass::ReadXMLAttributes(parent, element);
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
    int* initVal = new int[numElems];
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
      for(int i=0; i<numRead; i++)
        {
        this->SetElement(i, initVal[i]);
        }
      }
    else
      {
      vtkErrorMacro("No default value is specified for property: "
                    << this->GetXMLName()
                    << ". This might lead to stability problems");
      }
    delete[] initVal;
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::SaveState(
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
          << i << "\" " << "value=\"" << this->GetElement(i) << "\"/>"
          << endl;
    }
  this->Superclass::SaveState(name, file, indent);
  *file << indent << "</Property>" << endl;
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ArgumentIsArray: " << this->ArgumentIsArray << endl;
  os << indent << "Values: ";
  for (unsigned int i=0; i<this->GetNumberOfElements(); i++)
    {
    os << this->GetElement(i) << " ";
    }
  os << endl;
}
