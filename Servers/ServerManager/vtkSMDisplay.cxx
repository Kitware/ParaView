/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDisplay.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDisplay.h"


//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSMDisplay, "1.1");

//----------------------------------------------------------------------------
vtkSMDisplay::vtkSMDisplay()
{
}

//----------------------------------------------------------------------------
vtkSMDisplay::~vtkSMDisplay()
{
}

//----------------------------------------------------------------------------
void vtkSMDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkSMDisplay::MarkConsumersAsModified()
{
  this->Superclass::MarkConsumersAsModified();
  this->InvalidateGeometry();
}


