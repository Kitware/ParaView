/*=========================================================================

  Program:   ParaView
  Module:    vtkPMObject.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMObject.h"

#include "vtkObjectFactory.h"
#include "vtkSMSessionCore.h"

#include <assert.h>

//----------------------------------------------------------------------------
vtkPMObject::vtkPMObject()
{
  this->Interpreter = 0;
}

//----------------------------------------------------------------------------
vtkPMObject::~vtkPMObject()
{
}

//----------------------------------------------------------------------------
void vtkPMObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPMObject::Initialize(vtkSMSessionCore* session)
{
  assert(session != NULL);
  this->SessionCore = session;
  this->Interpreter = session->GetInterpreter();
}

//----------------------------------------------------------------------------
void vtkPMObject::Finalize()
{
  this->Interpreter = NULL;
  this->SessionCore = NULL;
}
