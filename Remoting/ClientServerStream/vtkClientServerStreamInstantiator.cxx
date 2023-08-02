// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkClientServerStreamInstantiator.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerInterpreterInitializer.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkClientServerStreamInstantiator);
//----------------------------------------------------------------------------
vtkClientServerStreamInstantiator::vtkClientServerStreamInstantiator() = default;

//----------------------------------------------------------------------------
vtkClientServerStreamInstantiator::~vtkClientServerStreamInstantiator() = default;

//----------------------------------------------------------------------------
vtkObjectBase* vtkClientServerStreamInstantiator::CreateInstance(const char* className)
{
  vtkClientServerInterpreter* interp = vtkClientServerInterpreterInitializer::GetInitializer()
    ? vtkClientServerInterpreterInitializer::GetInitializer()->GetGlobalInterpreter()
    : nullptr;
  return interp ? interp->NewInstance(className) : nullptr;
}

//----------------------------------------------------------------------------
void vtkClientServerStreamInstantiator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
