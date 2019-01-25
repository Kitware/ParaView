/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVInstantiator.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerInterpreterInitializer.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVInstantiator);
//----------------------------------------------------------------------------
vtkPVInstantiator::vtkPVInstantiator()
{
}

//----------------------------------------------------------------------------
vtkPVInstantiator::~vtkPVInstantiator()
{
}

vtkObject* vtkPVInstantiator::CreateInstance(const char* className)
{
  vtkClientServerInterpreter* interp = vtkClientServerInterpreterInitializer::GetInitializer()
    ? vtkClientServerInterpreterInitializer::GetInitializer()->GetGlobalInterpreter()
    : NULL;
  vtkObjectBase* objbase = interp ? interp->NewInstance(className) : NULL;
  vtkObject* obj = vtkObject::SafeDownCast(objbase);
  if (objbase != NULL && obj == NULL)
  {
    objbase->Delete();
    objbase = NULL;
  }
  return obj;
}

//----------------------------------------------------------------------------
void vtkPVInstantiator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
