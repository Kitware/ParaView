/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDeserializer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDeserializer.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"

//----------------------------------------------------------------------------
vtkSMDeserializer::vtkSMDeserializer()
{
  this->Session = 0;
}

//----------------------------------------------------------------------------
vtkSMDeserializer::~vtkSMDeserializer()
{
  this->SetSession(0);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMDeserializer::CreateProxy(const char* xmlgroup,
                                           const char* xmlname,
                                           const char* subname)
{
  vtkSMProxyManager* pxm = this->GetProxyManager();
  vtkSMProxy* proxy = pxm->NewProxy(xmlgroup, xmlname, subname);
  return proxy;
}

//----------------------------------------------------------------------------
void vtkSMDeserializer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkSMSession* vtkSMDeserializer::GetSession()
{
  return this->Session.GetPointer();
}
//----------------------------------------------------------------------------
void vtkSMDeserializer::SetSession(vtkSMSession* s)
{
  this->Session = s;
}
