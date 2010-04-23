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

struct vtkSMIntVectorPropertyInternals
{
  vtkstd::vector<int> Values;
  vtkstd::vector<int> UncheckedValues;
  vtkstd::vector<int> LastPushedValues; // These are the values that
      // were last pushed onto the server. These are used to generate
      // the undo/redo state.
  vtkstd::vector<int> DefaultValues; // Values set in the XML configuration.
  bool DefaultsValid;

  vtkSMIntVectorPropertyInternals() : DefaultsValid(false) {}
  void UpdateLastPushedValues()
    {
    // Update LastPushedValues.
    this->LastPushedValues.clear();
    this->LastPushedValues.insert(this->LastPushedValues.end(),
      this->Values.begin(), this->Values.end());
    }

  void UpdateDefaultValues()
    {
    this->DefaultValues.clear();
    this->DefaultValues.insert(this->DefaultValues.end(),
      this->Values.begin(), this->Values.end());
    this->DefaultsValid = true;
    }
};

//---------------------------------------------------------------------------
vtkSMIntVectorProperty::vtkSMIntVectorProperty()
{
  this->Internals = new vtkSMIntVectorPropertyInternals;
  this->ArgumentIsArray = 0;
  this->Initialized = false;
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
  if (this->InformationOnly || !this->Initialized)
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

  if (this->SetNumberCommand)
    {
    *str << vtkClientServerStream::Invoke 
         << objectId << this->SetNumberCommand 
         << this->GetNumberOfElements() / this->NumberOfElementsPerCommand
         << vtkClientServerStream::End;
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

  this->Internals->UpdateLastPushedValues();
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::SetNumberOfUncheckedElements(unsigned int num)
{
  this->Internals->UncheckedValues.resize(num, 0);
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::SetNumberOfElements(unsigned int num)
{
  if (num == this->Internals->Values.size())
    {
    return;
    }
  this->Internals->Values.resize(num, 0);
  this->Internals->UncheckedValues.resize(num, 0);
  if (num == 0)
    {
    // If num == 0, then we already have the intialized values (so to speak).
    this->Initialized = true;
    }
  else
    {
    this->Initialized = false;
    }
  this->Modified();
}

//---------------------------------------------------------------------------
unsigned int vtkSMIntVectorProperty::GetNumberOfUncheckedElements()
{
  return static_cast<unsigned int>(this->Internals->UncheckedValues.size());
}

//---------------------------------------------------------------------------
unsigned int vtkSMIntVectorProperty::GetNumberOfElements()
{
  return static_cast<unsigned int>(this->Internals->Values.size());
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::GetElement(unsigned int idx)
{
  return this->Internals->Values[idx];
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::GetDefaultValue(int idx)
{
  if (idx >= 0 && idx < (int)this->Internals->DefaultValues.size())
    {
    return this->Internals->DefaultValues[idx];
    }
  return 0;
}

//---------------------------------------------------------------------------
int *vtkSMIntVectorProperty::GetElements()
{
  return (this->Internals->Values.size() > 0)?
    &this->Internals->Values[0] : NULL;
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
  unsigned int numElems = this->GetNumberOfElements();

  if (this->Initialized && idx < numElems && value == this->GetElement(idx))
    {
    return 1;
    }

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
  
  if (idx >= numElems)
    {
    this->SetNumberOfElements(idx+1);
    }
  this->Internals->Values[idx] = value;
  // Make sure to initialize BEFORE Modified() is called. Otherwise,
  // the value would not be pushed.
  this->Initialized = true;
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
  unsigned int numArgs = this->GetNumberOfElements();

  int modified = 0;
  for (unsigned int i=0; i<numArgs; i++)
    {
    if (this->Internals->Values[i] != values[i])
      {
      modified = 1;
      break;
      }
    }
  if(!modified && this->Initialized)
    {
    return 1;
    }

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
  this->Initialized = true;
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
    if (element->GetAttribute("default_values") &&
        strcmp("none", element->GetAttribute("default_values")) == 0 )
      {
      this->Initialized = false;
      }
    else
      {
      int* initVal = new int[numElems];
      int numRead = element->GetVectorAttribute("default_values",
                                                numElems,
                                                initVal);
      
      if (numRead > 0)
        {
        if (numRead != numElems)
          {
          vtkErrorMacro("The number of default values does not match the "
                        "number of elements. Initialization failed.");
          delete[] initVal;
          return 0;
          }
        for(int i=0; i<numRead; i++)
          {
          this->SetElement(i, initVal[i]);
          }
        this->Internals->UpdateLastPushedValues();
        this->Internals->UpdateDefaultValues();
        }
      else if (!this->Initialized)
        {
        vtkErrorMacro("No default value is specified for property: "
                      << this->GetXMLName()
                      << ". This might lead to stability problems");
        }
      delete[] initVal;
      }
    }

  return 1;
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::LoadState(vtkPVXMLElement* element,
  vtkSMProxyLocator* loader, int loadLastPushedValues/*=0*/)
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

  bool prev = this->SetBlockModifiedEvents(true);
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
        int value;
        if (currentElement->GetScalarAttribute("value", &value))
          {
          this->SetElement(index, value);
          }
        }
      }
    }
  this->SetBlockModifiedEvents(prev);

  // Do not immediately update. Leave it to the loader.
  if (this->GetPendingModifiedEvents())
    {
    this->Modified();
    }
  this->ImmediateUpdate = prevImUpdate;

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::ChildSaveState(vtkPVXMLElement* propertyElement, 
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
    elementElement->AddAttribute("value", this->GetElement(i));
    propertyElement->AddNestedElement(elementElement);
    elementElement->Delete();
    }

  if (saveLastPushedValues)
    {
    size = static_cast<unsigned int>(this->Internals->LastPushedValues.size());
    
    vtkPVXMLElement* element = vtkPVXMLElement::New();
    element->SetName("LastPushedValues");
    element->AddAttribute("number_of_elements", size);
    for (unsigned int cc=0; cc < size; ++cc)
      {
      vtkPVXMLElement* elementElement = vtkPVXMLElement::New();
      elementElement->SetName("Element");
      elementElement->AddAttribute("index", cc);
      elementElement->AddAttribute("value", 
        this->Internals->LastPushedValues[cc]);
      element->AddNestedElement(elementElement);
      elementElement->Delete();
      }
    propertyElement->AddNestedElement(element);
    element->Delete();
    }
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::Copy(vtkSMProperty* src)
{
  this->Superclass::Copy(src);

  vtkSMIntVectorProperty* dsrc = vtkSMIntVectorProperty::SafeDownCast(
    src);
  if (dsrc && dsrc->Initialized)
    {
    bool modified = false;
    
    if (this->Internals->Values != dsrc->Internals->Values)
      {
      this->Internals->Values = dsrc->Internals->Values;
      modified = true;
      }
    // If we were not initialized, we are now modified even if the value
    // did not change
    modified = modified || !this->Initialized;
    this->Initialized = true;
  
    this->Internals->UncheckedValues = dsrc->Internals->UncheckedValues;
    if (modified)
      {
      this->Modified();
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::ResetToDefaultInternal()
{
  if (this->Internals->DefaultValues != this->Internals->Values &&
      this->Internals->DefaultsValid)
    {
    this->Internals->Values = this->Internals->DefaultValues;
    // Make sure to initialize BEFORE Modified() is called. Otherwise,
    // the value would not be pushed.
    this->Initialized = true;    
    this->Modified();
    }
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
