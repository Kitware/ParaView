/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkSMCommunicationModule.h"
#include "vtkSMProxy.h"

vtkStandardNewMacro(vtkSMProxyProperty);
vtkCxxRevisionMacro(vtkSMProxyProperty, "1.1");

//---------------------------------------------------------------------------
vtkSMProxyProperty::vtkSMProxyProperty()
{
  this->Proxy = 0;
}

//---------------------------------------------------------------------------
vtkSMProxyProperty::~vtkSMProxyProperty()
{
  if (this->Proxy)
    {
    this->Proxy->Delete();
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::SetProxy(vtkSMProxy* proxy)
{
  if (this->Proxy != proxy)
    {
    if (this->Proxy != NULL) { this->Proxy->UnRegister(this); }
    this->Proxy = proxy;
    if (this->Proxy != NULL) 
      { 
      this->Proxy->Register(this); 
      this->Proxy->CreateVTKObjects(1);
      }
    this->Modified();
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::AppendCommandToStream(
    vtkClientServerStream* str, vtkClientServerID objectId )
{
  if (!this->Command)
    {
    return;
    }

  *str << vtkClientServerStream::Invoke << objectId << this->Command;
  if (this->Proxy)
    {
    int numIDs = this->Proxy->GetNumberOfIDs();
    for(int i=0; i<numIDs; i++)
      {
      *str << this->Proxy->GetID(i);
      }
    }
  else
    {
    vtkClientServerID nullID = { 0 };
    *str << nullID;
    }
  *str << vtkClientServerStream::End;
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
