/*=========================================================================

  Program:   ParaView
  Module:    vtkCompositeDataVisitorCommand.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataVisitorCommand.h"

vtkCxxRevisionMacro(vtkCompositeDataVisitorCommand, "1.2");

//----------------------------------------------------------------------------
vtkCompositeDataVisitorCommand::vtkCompositeDataVisitorCommand()
{
}

//----------------------------------------------------------------------------
vtkCompositeDataVisitorCommand::~vtkCompositeDataVisitorCommand()
{
}

//----------------------------------------------------------------------------
void vtkCompositeDataVisitorCommand::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

