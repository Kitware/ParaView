/*=========================================================================

  Program:   ParaView
  Module:    vtkSIProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIProperty.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkSIProxy.h"
#include "vtkPVXMLElement.h"

vtkStandardNewMacro(vtkSIProperty);
//----------------------------------------------------------------------------
vtkSIProperty::vtkSIProperty()
{
  this->Command = NULL;
  this->XMLName = NULL;
  this->IsInternal = false;
  this->UpdateSelf = false;
  this->InformationOnly = false;
  this->Repeatable = false;
  this->SIProxyObject = NULL;
}

//----------------------------------------------------------------------------
vtkSIProperty::~vtkSIProperty()
{
  this->SetCommand(NULL);
  this->SetXMLName(NULL);
}

//----------------------------------------------------------------------------
bool vtkSIProperty::ReadXMLAttributes(
  vtkSIProxy* proxyhelper, vtkPVXMLElement* element)
{
  this->SIProxyObject = proxyhelper;

  const char* xmlname = element->GetAttribute("name");
  if(xmlname)
    {
    this->SetXMLName(xmlname);
    }

  const char* command = element->GetAttribute("command");
  if (command)
    {
    this->SetCommand(command);
    }

  int repeatable;
  if (element->GetScalarAttribute("repeatable", &repeatable))
    {
    this->Repeatable = (repeatable != 0);
    }

  // Yup, both mean the same thing :).
  int repeat_command;
  if (element->GetScalarAttribute("repeat_command", &repeat_command))
    {
    this->Repeatable = (repeat_command != 0);
    }


  int update_self;
  if (element->GetScalarAttribute("update_self", &update_self))
    {
    this->UpdateSelf = (update_self != 0);
    }

  int information_only;
  if (element->GetScalarAttribute("information_only", &information_only))
    {
    this->InformationOnly = (information_only != 0);
    }

  int is_internal;
  if (element->GetScalarAttribute("is_internal", &is_internal))
    {
    this->SetIsInternal(is_internal != 0);
    }

  return true;
}

//----------------------------------------------------------------------------
vtkSIObject* vtkSIProperty::GetSIObject(vtkTypeUInt32 globalid)
{
  if (this->SIProxyObject)
    {
    return this->SIProxyObject->GetSIObject(globalid);
    }
  return NULL;
}

//----------------------------------------------------------------------------
bool vtkSIProperty::ProcessMessage(vtkClientServerStream& stream)
{
  if (this->SIProxyObject && this->SIProxyObject->GetVTKObject())
    {
    return this->SIProxyObject->GetInterpreter()->ProcessStream(stream) != 0;
    }
  return this->SIProxyObject ? true : false;
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkSIProperty::GetVTKObject()
{
  if (this->SIProxyObject)
    {
    return this->SIProxyObject->GetVTKObject();
    }
  return NULL;
}

//----------------------------------------------------------------------------
const vtkClientServerStream& vtkSIProperty::GetLastResult()
{
  if (this->SIProxyObject)
    {
    return this->SIProxyObject->GetInterpreter()->GetLastResult();
    }

  static vtkClientServerStream stream;
  return stream;
}

//----------------------------------------------------------------------------
void vtkSIProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
// CAUTION: This method should only be called for Command property only
//          and not for value property.
bool vtkSIProperty::Push(vtkSMMessage*, int)
{
  if ( this->InformationOnly || !this->Command || this->UpdateSelf ||
       this->GetVTKObject() == NULL)
    {
    return true;
    }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke;
  stream << this->GetVTKObject() << this->Command;
  stream << vtkClientServerStream::End;
  return this->ProcessMessage(stream);
}
