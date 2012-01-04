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
#include "vtkSMSessionProxyManager.h"

#include <assert.h>

//----------------------------------------------------------------------------
vtkSMDeserializer::vtkSMDeserializer()
{
}

//----------------------------------------------------------------------------
vtkSMDeserializer::~vtkSMDeserializer()
{
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMDeserializer::CreateProxy(const char* xmlgroup,
                                           const char* xmlname,
                                           const char* subname)
{
  assert("Expect a valid session" && this->Session);
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  assert("Expect a valid SessionProxyManager" && pxm);
  vtkSMProxy* proxy = pxm->NewProxy(xmlgroup, xmlname, subname);
  return proxy;
}

//----------------------------------------------------------------------------
void vtkSMDeserializer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
