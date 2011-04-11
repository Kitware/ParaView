/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
* Copyright (c) 2007, Sandia Corporation
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the Sandia Corporation nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Sandia Corporation ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Sandia Corporation BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "vtkSMPropertyHelper.h"

#include "vtkObjectFactory.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkStringList.h"
#include "vtkSMEnumerationDomain.h"

#include <vtksys/ios/sstream>

#include <assert.h>
inline unsigned int vtkSMPropertyHelperMin(unsigned int x, unsigned int y)
{
  return x < y? x : y;
}

#define vtkSMPropertyHelperWarningMacro(blah)\
  if (this->Quiet == false) \
  {\
    vtkGenericWarningMacro(blah)\
  }


//----------------------------------------------------------------------------
vtkSMPropertyHelper::vtkSMPropertyHelper(vtkSMProxy* proxy, const char* pname,
  bool quiet)
{
  this->Proxy = proxy;
  this->Property = proxy->GetProperty(pname);
  this->Type = vtkSMPropertyHelper::NONE;
  this->DoubleValues = NULL;
  this->IntValues = NULL;
  this->IdTypeValues = NULL;
  this->Quiet = quiet;

  if (!this->Property)
    {
    vtkSMPropertyHelperWarningMacro("Failed to locate property: " << pname);
    }
  else if (this->Property->IsA("vtkSMIntVectorProperty"))
    {
    this->Type = vtkSMPropertyHelper::INT;
    }
  else if (this->Property->IsA("vtkSMDoubleVectorProperty"))
    {
    this->Type = vtkSMPropertyHelper::DOUBLE;
    }
  else if (this->Property->IsA("vtkSMIdTypeVectorProperty"))
    {
    this->Type = vtkSMPropertyHelper::IDTYPE;
    }
  else if (this->Property->IsA("vtkSMStringVectorProperty"))
    {
    this->Type = vtkSMPropertyHelper::STRING;
    }
  else if (this->Property->IsA("vtkSMInputProperty"))
    {
    this->Type = vtkSMPropertyHelper::INPUT;
    }
  else if (this->Property->IsA("vtkSMProxyProperty"))
    {
    this->Type = vtkSMPropertyHelper::PROXY;
    }
  else
    {
    vtkSMPropertyHelperWarningMacro("Unhandled property type : " << this->Property->GetClassName());
    }
}

//----------------------------------------------------------------------------
vtkSMPropertyHelper::~vtkSMPropertyHelper()
{
  delete [] this->IntValues;
  delete [] this->IdTypeValues;
  delete [] this->DoubleValues;
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::UpdateValueFromServer()
{
  this->Proxy->UpdatePropertyInformation(this->Property);
}

#define SM_CASE_MACRO(typeN, type, ctype, call) \
  case typeN: \
    { \
    typedef ctype VTK_TT; \
    type* SMProperty = static_cast<type*>(this->Property); \
    call; \
    }; \
  break

#define SM_TEMPLATE_MACRO_NUM(call) \
  SM_CASE_MACRO(vtkSMPropertyHelper::INT, vtkSMIntVectorProperty, int, call);\
  SM_CASE_MACRO(vtkSMPropertyHelper::DOUBLE, vtkSMDoubleVectorProperty, double, call);\
  SM_CASE_MACRO(vtkSMPropertyHelper::IDTYPE, vtkSMIdTypeVectorProperty, vtkIdType, call)

#define SM_TEMPLATE_MACRO_VP(call) \
  SM_TEMPLATE_MACRO_NUM(call); \
  SM_CASE_MACRO(vtkSMPropertyHelper::STRING, vtkSMIdTypeVectorProperty, const char*, call)

#define SM_TEMPLATE_MACRO_PP(call) \
  SM_CASE_MACRO(vtkSMPropertyHelper::PROXY, vtkSMProxyProperty, vtkSMProxy*, call);\
  SM_CASE_MACRO(vtkSMPropertyHelper::INPUT, vtkSMInputProperty, vtkSMProxy*, call)

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::SetNumberOfElements(unsigned int elems)
{
  switch (this->Type)
    {
    SM_TEMPLATE_MACRO_VP(SMProperty->SetNumberOfElements(elems));
    SM_TEMPLATE_MACRO_PP(SMProperty->SetNumberOfProxies(elems));
  default:
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }
}

//----------------------------------------------------------------------------
unsigned int vtkSMPropertyHelper::GetNumberOfElements()
{
  switch (this->Type)
    {
    SM_TEMPLATE_MACRO_VP(return SMProperty->GetNumberOfElements());
    SM_TEMPLATE_MACRO_PP(return SMProperty->GetNumberOfProxies());

  default:
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Set(unsigned int index, int value)
{
  switch (this->Type)
    {
    SM_TEMPLATE_MACRO_NUM(SMProperty->SetElement(index, static_cast<VTK_TT>(value)));
  default:
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }
}

//----------------------------------------------------------------------------
int vtkSMPropertyHelper::GetAsInt(unsigned int index /*=0*/)
{
  switch (this->Type)
    {
    SM_TEMPLATE_MACRO_NUM(
      return static_cast<int>(SMProperty->GetElement(index)));

  default:
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }

  return 0;
}

//----------------------------------------------------------------------------
unsigned int vtkSMPropertyHelper::Get(int *values, unsigned int count /*=1*/)
{
  switch (this->Type)
    {
    SM_TEMPLATE_MACRO_NUM(
      count = ::vtkSMPropertyHelperMin(SMProperty->GetNumberOfElements(), count);
      for (unsigned int cc=0; cc < count; cc++)
        {
        values[cc] = static_cast<int>(SMProperty->GetElement(cc));
        }
      return count;
    );

  default:
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }

  return 0;
}

//----------------------------------------------------------------------------
const int* vtkSMPropertyHelper::GetAsIntPtr()
{
  delete [] this->IntValues;
  this->IntValues = NULL;
  int num_elems = this->GetNumberOfElements();
  if (num_elems)
    {
    this->IntValues = new int[num_elems];
    this->Get(this->IntValues, num_elems);
    }
  return this->IntValues;
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Set(const int* values, unsigned int count)
{
  switch (this->Type)
    {
    SM_TEMPLATE_MACRO_NUM(
      SMProperty->SetNumberOfElements(count);
      VTK_TT* clone = new VTK_TT[count];
      for (unsigned int cc=0; cc < count; cc++)
        {
        clone[cc] = static_cast<VTK_TT>(values[cc]);
        }
      SMProperty->SetElements(clone);
      delete []clone;
    );

  default:
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Set(unsigned int index, double value)
{
  switch (this->Type)
    {
    SM_TEMPLATE_MACRO_NUM(SMProperty->SetElement(index, static_cast<VTK_TT>(value)));
  default:
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }
}

//----------------------------------------------------------------------------
double vtkSMPropertyHelper::GetAsDouble(unsigned int index /*=0*/)
{
  switch (this->Type)
    {
    SM_TEMPLATE_MACRO_NUM(
      return static_cast<double>(SMProperty->GetElement(index)));

  default:
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }

  return 0;
}

//----------------------------------------------------------------------------
unsigned int vtkSMPropertyHelper::Get(double *values, unsigned int count /*=1*/)
{
  switch (this->Type)
    {
    SM_TEMPLATE_MACRO_NUM(
      count = ::vtkSMPropertyHelperMin(SMProperty->GetNumberOfElements(), count);
      for (unsigned int cc=0; cc < count; cc++)
        {
        values[cc] = static_cast<double>(SMProperty->GetElement(cc));
        }
      return count;
    );

  default:
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }

  return 0;
}

//----------------------------------------------------------------------------
const double* vtkSMPropertyHelper::GetAsDoublePtr()
{
  delete [] this->DoubleValues;
  this->DoubleValues = NULL;
  int num_elems = this->GetNumberOfElements();
  if (num_elems)
    {
    this->DoubleValues= new double[num_elems];
    this->Get(this->DoubleValues, num_elems);
    }
  return this->DoubleValues;
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Set(const double* values, unsigned int count)
{
  switch (this->Type)
    {
    SM_TEMPLATE_MACRO_NUM(
      SMProperty->SetNumberOfElements(count);
      VTK_TT* clone = new VTK_TT[count];
      for (unsigned int cc=0; cc < count; cc++)
        {
        clone[cc] = static_cast<VTK_TT>(values[cc]);
        }
      SMProperty->SetElements(clone);
      delete []clone;
    );

  default:
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }
}

#if VTK_SIZEOF_ID_TYPE != VTK_SIZEOF_INT
//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Set(unsigned int index, vtkIdType value)
{
  switch (this->Type)
    {
    SM_TEMPLATE_MACRO_NUM(SMProperty->SetElement(index, static_cast<VTK_TT>(value)));

  default:
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Set(const vtkIdType* values, unsigned int count)
{
  switch (this->Type)
    {
    SM_TEMPLATE_MACRO_NUM(
      SMProperty->SetNumberOfElements(count);
      VTK_TT* clone = new VTK_TT[count];
      for (unsigned int cc=0; cc < count; cc++)
        {
        clone[cc] = static_cast<VTK_TT>(values[cc]);
        }
      SMProperty->SetElements(clone);
      delete []clone;
    );

  default:
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }
}

//----------------------------------------------------------------------------
unsigned int vtkSMPropertyHelper::Get(vtkIdType* values, unsigned int count /*=1*/)
{
  switch (this->Type)
    {
    SM_TEMPLATE_MACRO_NUM(
      count = ::vtkSMPropertyHelperMin(SMProperty->GetNumberOfElements(), count);
      for (unsigned int cc=0; cc < count; cc++)
        {
        values[cc] = static_cast<vtkIdType>(SMProperty->GetElement(cc));
        }
      return count;
    );

  default:
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }

  return 0;
}
#endif

//----------------------------------------------------------------------------
vtkIdType vtkSMPropertyHelper::GetAsIdType(unsigned int index /*=0*/)
{
  switch (this->Type)
    {
    SM_TEMPLATE_MACRO_NUM(
      return static_cast<vtkIdType>(SMProperty->GetElement(index)));

  default:
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }

  return 0;
}

//----------------------------------------------------------------------------
const vtkIdType* vtkSMPropertyHelper::GetAsIdTypePtr()
{
  delete [] this->IdTypeValues;
  this->IdTypeValues = NULL;
  int num_elems = this->GetNumberOfElements();
  if (num_elems)
    {
    this->IdTypeValues = new vtkIdType[num_elems];
    this->Get(this->IdTypeValues, num_elems);
    }
  return this->IdTypeValues;
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Set(unsigned int index, const char* value)
{
  if (this->Type == vtkSMPropertyHelper::STRING)
    {
    vtkSMStringVectorProperty* svp = static_cast<vtkSMStringVectorProperty*>(
      this->Property);
    svp->SetElement(index, value);
    }
  else if (this->Type == vtkSMPropertyHelper::INT)
    {
    // enumeration domain
    vtkSMEnumerationDomain* domain = vtkSMEnumerationDomain::SafeDownCast(
      this->Property->FindDomain("vtkSMEnumerationDomain"));
    if (domain != NULL && domain->HasEntryText(value))
      {
      vtkSMIntVectorProperty* ivp = static_cast<vtkSMIntVectorProperty*>(
        this->Property);
      int valid; // We already know that the entry exist...
      ivp->SetElement(index, domain->GetEntryValue(value, valid));
      }
    else
      {
      vtkSMPropertyHelperWarningMacro(
        "'" << value <<"' could not be converted to int.");
      }
    }
  else
    {
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }
}

//----------------------------------------------------------------------------
const char* vtkSMPropertyHelper::GetAsString(unsigned int index /*=0*/)
{
  if (this->Type == vtkSMPropertyHelper::STRING)
    {
    vtkSMStringVectorProperty* svp = static_cast<vtkSMStringVectorProperty*>(
      this->Property);
    return svp->GetElement(index);
    }
  else if (this->Type == vtkSMPropertyHelper::INT)
    {
    // enumeration domain
    vtkSMEnumerationDomain* domain = vtkSMEnumerationDomain::SafeDownCast(
      this->Property->FindDomain("vtkSMEnumerationDomain"));
    if (domain != NULL)
      {
      vtkSMIntVectorProperty* ivp = static_cast<vtkSMIntVectorProperty*>(
        this->Property);
      const char* entry = domain->GetEntryTextForValue(ivp->GetElement(index));
      if (entry)
        {
        return entry;
        }
      }
    }

  vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
  return 0;
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Set(unsigned int index, vtkSMProxy* value, 
  unsigned int outputport/*=0*/)
{
  if (this->Type == vtkSMPropertyHelper::PROXY)
    {
    vtkSMProxyProperty* pp = static_cast<vtkSMProxyProperty*>(this->Property);
    pp->SetProxy(index, value);
    }
  else if (this->Type == vtkSMPropertyHelper::INPUT)
    {
    vtkSMInputProperty* ip = static_cast<vtkSMInputProperty*>(this->Property);
    ip->SetInputConnection(index, value, outputport);
    }
  else
    {
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Set(vtkSMProxy** value, unsigned int count, 
  unsigned int *outputports/*=NULL*/)
{
  if (this->Type == vtkSMPropertyHelper::PROXY)
    {
    vtkSMProxyProperty* pp = static_cast<vtkSMProxyProperty*>(this->Property);
    pp->SetProxies(count, value);
    }
  else if (this->Type == vtkSMPropertyHelper::INPUT)
    {
    vtkSMInputProperty* ip = static_cast<vtkSMInputProperty*>(this->Property);
    ip->SetProxies(count, value, outputports);
    }
  else
    {
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Add(vtkSMProxy* value, unsigned int outputport/*=0*/)
{
  if (this->Type == vtkSMPropertyHelper::PROXY)
    {
    vtkSMProxyProperty* pp = static_cast<vtkSMProxyProperty*>(this->Property);
    pp->AddProxy(value);
    }
  else if (this->Type == vtkSMPropertyHelper::INPUT)
    {
    vtkSMInputProperty* ip = static_cast<vtkSMInputProperty*>(this->Property);
    ip->AddInputConnection(value, outputport);
    }
  else
    {
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Remove(vtkSMProxy* value)
{
  if (this->Type == vtkSMPropertyHelper::PROXY ||
    this->Type == vtkSMPropertyHelper::INPUT)
    {
    vtkSMProxyProperty* pp = static_cast<vtkSMProxyProperty*>(this->Property);
    pp->RemoveProxy(value);
    }
  else
    {
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMPropertyHelper::GetAsProxy(unsigned int index/*=0*/)
{
  switch (this->Type)
    {
  case vtkSMPropertyHelper::PROXY:
  case vtkSMPropertyHelper::INPUT:
    {
    vtkSMProxyProperty* pp = static_cast<vtkSMProxyProperty*>(this->Property);
    return pp->GetProxy(index);
    }

  default:
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
    }
  return 0;
}

//----------------------------------------------------------------------------
unsigned int vtkSMPropertyHelper::GetOutputPort(unsigned int index/*=0*/)
{
  if (this->Type == vtkSMPropertyHelper::INPUT)
    {
    vtkSMInputProperty* ip = static_cast<vtkSMInputProperty*>(this->Property);
    return ip->GetOutputPortForConnection(index);
    }

  vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
  return 0;
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::SetStatus(const char* key, int value)
{
  if (this->Type != vtkSMPropertyHelper::STRING)
    {
    vtkSMPropertyHelperWarningMacro("Status properties can only be vtkSMStringVectorProperty.");
    return;
    }

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Property);
  if (svp->GetNumberOfElementsPerCommand() != 2)
    {
    vtkSMPropertyHelperWarningMacro("NumberOfElementsPerCommand != 2");
    return;
    }

  if (!svp->GetRepeatCommand())
    {
    vtkSMPropertyHelperWarningMacro("Property is non-repeatable.");
    return;
    }


  vtksys_ios::ostringstream str;
  str << value;

  for (unsigned int cc=0; (cc+1) < svp->GetNumberOfElements(); cc+=2)
    {
    if (strcmp(svp->GetElement(cc), key) == 0)
      {
      svp->SetElement(cc+1, str.str().c_str());
      return;
      }
    }

  vtkStringList* list = vtkStringList::New();
  svp->GetElements(list);
  list->AddString(key);
  list->AddString(str.str().c_str());
  svp->SetElements(list);
  list->Delete();
}

//----------------------------------------------------------------------------
int vtkSMPropertyHelper::GetStatus(const char* key, int default_value/*=0*/)
{
  if (this->Type != vtkSMPropertyHelper::STRING)
    {
    vtkSMPropertyHelperWarningMacro("Status properties can only be vtkSMStringVectorProperty.");
    return default_value;
    }

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Property);
  while (svp)
    {
    if (svp->GetNumberOfElementsPerCommand() != 2)
      {
      vtkSMPropertyHelperWarningMacro("NumberOfElementsPerCommand != 2");
      return default_value;
      }

    if (!svp->GetRepeatCommand())
      {
      vtkSMPropertyHelperWarningMacro("Property is non-repeatable.");
      return default_value;
      }

    for (unsigned int cc=0; (cc+1) < svp->GetNumberOfElements(); cc+=2)
      {
      if (strcmp(svp->GetElement(cc), key) == 0)
        {
        return atoi(svp->GetElement(cc+1));
        }
      }

    // Now check if the information_property has the value.
    svp = svp->GetInformationOnly() == 0? 
      vtkSMStringVectorProperty::SafeDownCast(svp->GetInformationProperty()) : 0;
    }

  return default_value;
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::SetStatus(const char* key, double *values,
  int num_values)
{
  if (this->Type != vtkSMPropertyHelper::STRING)
    {
    vtkSMPropertyHelperWarningMacro("Status properties can only be vtkSMStringVectorProperty.");
    return;
    }

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Property);
  if (svp->GetNumberOfElementsPerCommand() != num_values+1)
    {
    vtkSMPropertyHelperWarningMacro("NumberOfElementsPerCommand != " << num_values + 1);
    return;
    }

  if (!svp->GetRepeatCommand())
    {
    vtkSMPropertyHelperWarningMacro("Property is non-repeatable.");
    return;
    }


  vtkStringList* list = vtkStringList::New();
  svp->GetElements(list);

  bool append = true;
  for (unsigned int cc=0; (cc+num_values+1) <= svp->GetNumberOfElements();
    cc+=(num_values+1))
    {
    if (strcmp(svp->GetElement(cc), key) == 0)
      {
      for (int kk=0; kk < num_values; kk++)
        {
        vtksys_ios::ostringstream str;
        str << values[kk];
        list->SetString(cc+kk+1, str.str().c_str());
        }
      append = false;
      }
    }

  if (append)
    {
    list->AddString(key);
    for (int kk=0; kk < num_values; kk++)
      {
      vtksys_ios::ostringstream str;
      str << values[kk];
      list->AddString(str.str().c_str());
      }
    }
  svp->SetElements(list);
  list->Delete();
}

//----------------------------------------------------------------------------
bool vtkSMPropertyHelper::GetStatus(const char* key, double *values, int num_values)
{
  if (this->Type != vtkSMPropertyHelper::STRING)
    {
    vtkSMPropertyHelperWarningMacro("Status properties can only be vtkSMStringVectorProperty.");
    return false;
    }

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Property);

  while (svp)
    {
    if (svp->GetNumberOfElementsPerCommand() != num_values+1)
      {
      vtkSMPropertyHelperWarningMacro("NumberOfElementsPerCommand != " << num_values + 1);
      return false;
      }

    if (!svp->GetRepeatCommand())
      {
      vtkSMPropertyHelperWarningMacro("Property is non-repeatable.");
      return false;
      }

    for (unsigned int cc=0; (cc+num_values+1) <= svp->GetNumberOfElements();
      cc+=(num_values+1))
      {
      if (strcmp(svp->GetElement(cc), key) == 0)
        {
        for (int kk=0; kk < num_values; kk++)
          {
          values[kk] = atof(svp->GetElement(cc+kk+1));
          }
        return true;
        }
      }

    // Now check if the information_property has the value.
    svp = svp->GetInformationOnly() == 0? 
      vtkSMStringVectorProperty::SafeDownCast(svp->GetInformationProperty()) : 0;
    }

  return false;
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::SetStatus(const char* key, const char* value)
{
  if (this->Type != vtkSMPropertyHelper::STRING)
    {
    vtkSMPropertyHelperWarningMacro("Status properties can only be vtkSMStringVectorProperty.");
    return;
    }

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Property);
  if (svp->GetNumberOfElementsPerCommand() != 2)
    {
    vtkSMPropertyHelperWarningMacro("NumberOfElementsPerCommand != 2");
    return;
    }

  if (!svp->GetRepeatCommand())
    {
    vtkSMPropertyHelperWarningMacro("Property is non-repeatable.");
    return;
    }

  for (unsigned int cc=0; (cc+1) < svp->GetNumberOfElements(); cc+=2)
    {
    if (strcmp(svp->GetElement(cc), key) == 0)
      {
      svp->SetElement(cc+1, value);
      return;
      }
    }

  vtkStringList* list = vtkStringList::New();
  svp->GetElements(list);
  list->AddString(key);
  list->AddString(value);
  svp->SetElements(list);
  list->Delete();
}

//----------------------------------------------------------------------------
const char* vtkSMPropertyHelper::GetStatus(const char* key, const char* default_value)
{
  if (this->Type != vtkSMPropertyHelper::STRING)
    {
    vtkSMPropertyHelperWarningMacro("Status properties can only be vtkSMStringVectorProperty.");
    return default_value;
    }

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Property);
  while (svp)
    {
    if (svp->GetNumberOfElementsPerCommand() != 2)
      {
      vtkSMPropertyHelperWarningMacro("NumberOfElementsPerCommand != 2");
      return default_value;
      }

    if (!svp->GetRepeatCommand())
      {
      vtkSMPropertyHelperWarningMacro("Property is non-repeatable.");
      return default_value;
      }

    for (unsigned int cc=0; (cc+1) < svp->GetNumberOfElements(); cc+=2)
      {
      if (strcmp(svp->GetElement(cc), key) == 0)
        {
        return svp->GetElement(cc+1);
        }
      }

    // Now check if the information_property has the value.
    svp = svp->GetInformationOnly() == 0? 
      vtkSMStringVectorProperty::SafeDownCast(svp->GetInformationProperty()) : 0;
    }

  return default_value;
}

