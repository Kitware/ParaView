/*=========================================================================

  Program:   ParaView
  Module:    vtkClientServerStreamInstantiator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClientServerStreamInstantiator.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerInterpreterInitializer.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkClientServerStreamInstantiator);
//----------------------------------------------------------------------------
vtkClientServerStreamInstantiator::vtkClientServerStreamInstantiator()
{
}

//----------------------------------------------------------------------------
vtkClientServerStreamInstantiator::~vtkClientServerStreamInstantiator()
{
}

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
