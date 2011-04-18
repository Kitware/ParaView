/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLoadStateContext.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMLoadStateContext.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMLoadStateContext);
//----------------------------------------------------------------------------
vtkSMLoadStateContext::vtkSMLoadStateContext()
{
  this->LoadDefinitionOnly = false;
  this->RequestOrigin = UNDEFINED;
}

//----------------------------------------------------------------------------
vtkSMLoadStateContext::~vtkSMLoadStateContext()
{
}

//----------------------------------------------------------------------------
void vtkSMLoadStateContext::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LoadDefinitionOnly: " << this->LoadDefinitionOnly << endl;
  os << indent << "RequestOrigin: " << this->RequestOrigin << endl;
}
