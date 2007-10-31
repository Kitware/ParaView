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
vtkCxxRevisionMacro(vtkSMStringVectorProperty, "1.34");

struct vtkSMStringVectorPropertyInternals
{
  vtkstd::vector<vtkStdString> Values;
  vtkstd::vector<vtkStdString> UncheckedValues;
  vtkstd::vector<vtkStdString> LastPushedValues;
  vtkstd::vector<int> ElementTypes;
  vtkstd::vector<char> Initialized;

  void UpdateLastPushedValues()
    {
    // Save LastPushedValues.
    this->LastPushedValues.clear();
    this->LastPushedValues.insert(this->LastPushedValues.end(),
      this->Values.begin(), this->Values.end());
    }

  void Resize(unsigned int num)
    {
    this->Values.resize(num);
    this->Initialized.resize(num, 0);
    this->UncheckedValues.resize(num);
    }
};

//---------------------------------------------------------------------------
vtkSMStringVectorProperty::vtkSMStringVectorProperty()
{
  this->Internals = new vtkSMStringVectorPropertyInternals;
  this->Initialized = false;
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
  if (this->InformationOnly)
    {
    return;
    }

  if (!this->Command)
    {
    this->Internals->UpdateLastPushedValues();
    return;
    }

  if (this->CleanCommand)
    {
    *str << vtkClientServerStream::Invoke
      << objectId << this->CleanCommand
      << vtkClientServerStream::End;
    }

  // If none of the strings are initialized, don't send them.
  int i;
  int numArgs = this->GetNumberOfElements();
  int numInitArgs = 0;
  for(i=0; i<numArgs; i++)
    {
    if (this->Internals->Initialized[i])
      {
      numInitArgs++;
      }
    }
  if (numInitArgs == 0)
    {
    return;
    }

  if (!this->RepeatCommand)
    {
    *str << vtkClientServerStream::Invoke << objectId << this->Command;
    for(i=0; i<numArgs; i++)
      {
      // Convert to the appropriate type and add to stream
      switch (this->GetElementType(i))
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
    int numCommands = numArgs / this->NumberOfElementsPerCommand;
    for(i=0; i<numCommands; i++)
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
  this->Internals->UpdateLastPushedValues();
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::SetNumberOfUncheckedElements(unsigned int num)
{
  this->Internals->UncheckedValues.resize(num);
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::SetNumberOfElements(unsigned int num)
{
  if (num == this->Internals->Values.size())
    {
    return;
    }
  this->Internals->Resize(num);
  this->Initialized = false;
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
int vtkSMStringVectorProperty::SetElements(unsigned int count, const char* values[])
{
  unsigned int numElems = this->GetNumberOfElements();

  if (this->Initialized && count == numElems)
    {
    // Check is cur values are same as the new values.
    int modified = 0;
    for (unsigned int cc=0; cc < numElems; cc++)
      {
      const char* value = values[cc]? values[cc] : "";
      if (this->Internals->Values[cc] != value)
        {
        modified = 1;
        break;
        }
      }
    if (!modified)
      {
      // nothing changed.
      return 1;
      }
    }

  if ( vtkSMProperty::GetCheckDomains() )
    {
    this->SetNumberOfUncheckedElements(count);
    for(unsigned int cc=0; cc<count; cc++)
      {
      this->SetUncheckedElement(cc, values[cc]?values[cc]:"");
      }
    
    if (!this->IsInDomains())
      {
      this->SetNumberOfUncheckedElements(this->GetNumberOfElements());
      return 0;
      }
    }

  this->Internals->Resize(count);
  for (unsigned int cc=0; cc < count; cc++)
    {
    this->Internals->Values[cc] = values[cc]? values[cc]: "";
    this->Internals->Initialized[cc] = 1;
    }
  this->Initialized = true;
  this->Modified();
  return 1;
}

//---------------------------------------------------------------------------
int vtkSMStringVectorProperty::SetElement(unsigned int idx, const char* value)
{
  if (!value)
    {
    value = "";
    }

  unsigned int numElems = this->GetNumberOfElements();

  if (this->Initialized && 
      idx < numElems && strcmp(value, this->GetElement(idx)) == 0)
    {
    return 1;
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
  this->Internals->Initialized[idx] = 1;
  this->Modified();
  this->Initialized = true;
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

  int numEls = this->GetNumberOfElements();

  if (this->RepeatCommand)
    {
    numEls = this->GetNumberOfElementsPerCommand();
    }
  int* eTypes = new int[numEls];

  int numElsRead = element->GetVectorAttribute("element_types", numEls, eTypes);
  for (int i=0; i<numElsRead; i++)
    {
    this->Internals->ElementTypes.push_back(eTypes[i]);
    }
  delete[] eTypes;

  numEls = this->GetNumberOfElements();
  if (numEls > 0)
    {
    const char *initVal = element->GetAttribute("default_values");
    if (initVal)
      {
      this->SetElement(0, initVal); // what to do with > 1 element?
      }
    this->Internals->UpdateLastPushedValues(); 
    }
  return 1;
}

//---------------------------------------------------------------------------
int vtkSMStringVectorProperty::LoadState(vtkPVXMLElement* element,
  vtkSMStateLoaderBase* loader, int loadLastPushedValues/*=0*/)
{
  int prevImUpdate = this->ImmediateUpdate;

  // Wait until all values are set before update (if ImmediateUpdate)
  this->ImmediateUpdate = 0;
  this->Superclass::LoadState(element, loader, loadLastPushedValues);

  if (loadLastPushedValues)
    {
    unsigned int numElems = element->GetNumberOfNestedElements();
    vtkPVXMLElement* actual_element = NULL;
    for (unsigned int i=0; i < numElems; i++)
      {
      vtkPVXMLElement* currentElement = element->GetNestedElement(i);
      if (currentElement->GetName() && 
        strcmp(currentElement->GetName(), "LastPushedValues") == 0)
        {
        actual_element = currentElement;
        break;
        }
      }
    if (!actual_element)
      {
      // No LastPushedValues present, do nothing.
      return 1;
      }
    element = actual_element;
    }
  
  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = element->GetNestedElement(i);
    if (currentElement->GetName() &&
        strcmp(currentElement->GetName(), "Element") == 0)
      {
      int index;
      if (currentElement->GetScalarAttribute("index", &index))
        {
        char* value = currentElement->GetSanitizedAttribute("value");
        if (value)
          {
          this->SetElement(index, value);
          delete[] value;
          }
        }
      }
    }

  // Do not immediately update. Leave it to the loader.
  this->Modified();
  this->ImmediateUpdate = prevImUpdate;

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::ChildSaveState(vtkPVXMLElement* propertyElement,
  int saveLastPushedValues)
{
  this->Superclass::ChildSaveState(propertyElement, saveLastPushedValues);

  unsigned int size = this->GetNumberOfElements();
  if (size > 0)
    {
    propertyElement->AddAttribute("number_of_elements", size);
    }
  for (unsigned int i=0; i<size; i++)
    {
    vtkPVXMLElement* elementElement = vtkPVXMLElement::New();
    elementElement->SetName("Element");
    elementElement->AddAttribute("index", i);
    elementElement->AddSanitizedAttribute("value", 
                                          (this->GetElement(i)?this->GetElement(i):""));
    propertyElement->AddNestedElement(elementElement);
    elementElement->Delete();
    }

  if (saveLastPushedValues)
    {
    size = this->Internals->LastPushedValues.size();
    
    vtkPVXMLElement* element = vtkPVXMLElement::New();
    element->SetName("LastPushedValues");
    element->AddAttribute("number_of_elements", size);
    for (unsigned int cc=0; cc < size; ++cc)
      {
      vtkPVXMLElement* elementElement = vtkPVXMLElement::New();
      elementElement->SetName("Element");
      elementElement->AddAttribute("index", cc);
      elementElement->AddSanitizedAttribute("value", 
        this->Internals->LastPushedValues[cc]);
      element->AddNestedElement(elementElement);
      elementElement->Delete();
      }
    propertyElement->AddNestedElement(element);
    element->Delete();
    }
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::Copy(vtkSMProperty* src)
{
  this->Superclass::Copy(src);

  vtkSMStringVectorProperty* dsrc = vtkSMStringVectorProperty::SafeDownCast(
    src);
  if (dsrc)
    {
    int imUpdate = this->ImmediateUpdate;
    this->ImmediateUpdate = 0;
    unsigned int i;
    unsigned int numElems = dsrc->GetNumberOfElements();
    this->SetNumberOfElements(numElems);
    for(i=0; i<numElems; i++)
      {
      this->SetElement(i, dsrc->GetElement(i));
      }
    numElems = dsrc->GetNumberOfElements();
    this->SetNumberOfUncheckedElements(numElems);
    for(i=0; i<numElems; i++)
      {
      this->SetUncheckedElement(i, dsrc->GetUncheckedElement(i));
      }
    this->ImmediateUpdate = imUpdate;
    }

  this->Modified();
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
