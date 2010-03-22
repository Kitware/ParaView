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
#include "vtkProcessModule.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMDoubleVectorProperty);
vtkCxxRevisionMacro(vtkSMDoubleVectorProperty, "1.48");

struct vtkSMDoubleVectorPropertyInternals
{
  vtkstd::vector<double> Values;
  vtkstd::vector<double> UncheckedValues;
  vtkstd::vector<double> LastPushedValues;
  vtkstd::vector<double> DefaultValues;
  bool DefaultsValid;

  vtkSMDoubleVectorPropertyInternals() : DefaultsValid(false) {}

  void UpdateLastPushedValues()
    {
    // Set the last pushed values.
    this->LastPushedValues.clear();
    this->LastPushedValues.insert(
      this->LastPushedValues.end(),
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
vtkSMDoubleVectorProperty::vtkSMDoubleVectorProperty()
{
  this->Internals = new vtkSMDoubleVectorPropertyInternals;
  this->ArgumentIsArray = 0;
  this->Initialized = false;
  this->Precision = 0;
}

//---------------------------------------------------------------------------
vtkSMDoubleVectorProperty::~vtkSMDoubleVectorProperty()
{
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::UpdateLastPushedValues()
{
  this->Internals->UpdateLastPushedValues();
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::AppendCommandToStream(
  vtkSMProxy* vtkNotUsed(proxy), vtkClientServerStream* str, 
  vtkClientServerID objectId )
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
void vtkSMDoubleVectorProperty::SetNumberOfUncheckedElements(unsigned int num)
{
  this->Internals->UncheckedValues.resize(num, 0);
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::SetNumberOfElements(unsigned int num)
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
unsigned int vtkSMDoubleVectorProperty::GetNumberOfUncheckedElements()
{
  return this->Internals->UncheckedValues.size();
}

//---------------------------------------------------------------------------
unsigned int vtkSMDoubleVectorProperty::GetNumberOfElements()
{
  return this->Internals->Values.size();
}

//---------------------------------------------------------------------------
double* vtkSMDoubleVectorProperty::GetElements()
{
  return (this->Internals->Values.size() > 0) ?
    &this->Internals->Values[0] : NULL;
}

//---------------------------------------------------------------------------
double vtkSMDoubleVectorProperty::GetElement(unsigned int idx)
{
  return this->Internals->Values[idx];
}

//---------------------------------------------------------------------------
double vtkSMDoubleVectorProperty::GetUncheckedElement(unsigned int idx)
{
  return this->Internals->UncheckedValues[idx];
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::SetUncheckedElement(
  unsigned int idx, double value)
{
  if (idx >= this->GetNumberOfUncheckedElements())
    {
    this->SetNumberOfUncheckedElements(idx+1);
    }
  this->Internals->UncheckedValues[idx] = value;
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::SetElement(unsigned int idx, double value)
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
           numArgs*sizeof(double));
    
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
int vtkSMDoubleVectorProperty::SetElements1(double value0)
{
  return this->SetElement(0, value0);
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::SetElements2(double value0, double value1)
{
  int retVal1 = this->SetElement(0, value0);
  int retVal2 = this->SetElement(1, value1);
  return (retVal1 && retVal2);
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::SetElements3(
  double value0, double value1, double value2)
{
  int retVal1 = this->SetElement(0, value0);
  int retVal2 = this->SetElement(1, value1);
  int retVal3 = this->SetElement(2, value2);
  return (retVal1 && retVal2 && retVal3);
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::SetElements4(
  double value0, double value1, double value2, double value3)
{
  int retVal1 = this->SetElement(0, value0);
  int retVal2 = this->SetElement(1, value1);
  int retVal3 = this->SetElement(2, value2);
  int retVal4 = this->SetElement(3, value3);
  return (retVal1 && retVal2 && retVal3 && retVal4);
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::SetElements(const double* values,
  unsigned int numValues)
{
  unsigned int numArgs = this->GetNumberOfElements();

  int modified = (numArgs != numValues);
  for (unsigned int i=0; modified == 0 && i<numArgs; i++)
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
           numValues*sizeof(double));
    if (!this->IsInDomains())
      {
      return 0;
      }
    }

  this->Internals->Values.resize(numValues, 0);
  if (numValues > 0)
    {
    memcpy(&this->Internals->Values[0], values, numValues*sizeof(double));
    }
  this->Initialized = true;
  this->Modified();
  return 1;
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::SetElements(const double* values)
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
           numArgs*sizeof(double));
    if (!this->IsInDomains())
      {
      return 0;
      }
    }

  memcpy(&this->Internals->Values[0], values, numArgs*sizeof(double));
  this->Initialized = true;
  this->Modified();
  return 1;
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::ReadXMLAttributes(vtkSMProxy* proxy,
                                                 vtkPVXMLElement* element)
{
  int retVal;

  retVal = this->Superclass::ReadXMLAttributes(proxy, element);
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

  int precision=0;
  if (element->GetScalarAttribute("precision", &precision))
    {
    this->SetPrecision(precision);
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
int vtkSMDoubleVectorProperty::LoadState(vtkPVXMLElement* element,
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
        double value;
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
void vtkSMDoubleVectorProperty::ChildSaveState(
  vtkPVXMLElement* propertyElement, int saveLastPushedValues)
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
    elementElement->AddAttribute("value", this->GetElement(i),
      this->Precision);
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
      elementElement->AddAttribute("value", 
        this->Internals->LastPushedValues[cc],
        this->Precision);
      element->AddNestedElement(elementElement);
      elementElement->Delete();
      }
    propertyElement->AddNestedElement(element);
    element->Delete();
    }
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::Copy(vtkSMProperty* src)
{
  this->Superclass::Copy(src);

  vtkSMDoubleVectorProperty* dsrc = vtkSMDoubleVectorProperty::SafeDownCast(
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
void vtkSMDoubleVectorProperty::ResetToDefaultInternal()
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
void vtkSMDoubleVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ArgumentIsArray: " << this->ArgumentIsArray << endl;
  os << indent << "Precision: " << this->Precision << endl;

  os << indent << "Values: ";
  for (unsigned int i=0; i<this->GetNumberOfElements(); i++)
    {
    os << this->GetElement(i) << " ";
    }
  os << endl;
}
