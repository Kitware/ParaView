/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiplexerInputDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMMultiplexerInputDomain.h"

#include "vtkObjectFactory.h"
#include "vtkSMInputProperty.h"
#include "vtkSMMultiplexerSourceProxy.h"

vtkStandardNewMacro(vtkSMMultiplexerInputDomain);
//----------------------------------------------------------------------------
vtkSMMultiplexerInputDomain::vtkSMMultiplexerInputDomain() = default;

//----------------------------------------------------------------------------
vtkSMMultiplexerInputDomain::~vtkSMMultiplexerInputDomain() = default;

//----------------------------------------------------------------------------
int vtkSMMultiplexerInputDomain::IsInDomain(vtkSMProperty* property)
{
  auto mux = vtkSMMultiplexerSourceProxy::SafeDownCast(property->GetParent());
  auto ip = vtkSMInputProperty::SafeDownCast(property);
  return (mux && ip) ? mux->IsInDomain(ip) : vtkSMDomain::NOT_IN_DOMAIN;
}

//----------------------------------------------------------------------------
void vtkSMMultiplexerInputDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
