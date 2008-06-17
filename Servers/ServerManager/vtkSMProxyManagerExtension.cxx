/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyManagerExtension.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyManagerExtension.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"

vtkCxxRevisionMacro(vtkSMProxyManagerExtension, "1.1");
//----------------------------------------------------------------------------
vtkSMProxyManagerExtension::vtkSMProxyManagerExtension()
{
}

//----------------------------------------------------------------------------
vtkSMProxyManagerExtension::~vtkSMProxyManagerExtension()
{
}


//----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxyManagerExtension::GetProxyElement(
  const char* vtkNotUsed(groupName),
  const char* vtkNotUsed(proxyName), 
  vtkPVXMLElement* currentElement)
{
  return currentElement;
}

//----------------------------------------------------------------------------
void vtkSMProxyManagerExtension::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


