// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
