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
#include "vtkPVWebInteractionEvent.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVWebInteractionEvent);
//----------------------------------------------------------------------------
vtkPVWebInteractionEvent::vtkPVWebInteractionEvent() :
  Buttons(0),
  Modifiers(0),
  KeyCode(0),
  X(0.0),
  Y(0.0)
{
}

//----------------------------------------------------------------------------
vtkPVWebInteractionEvent::~vtkPVWebInteractionEvent()
{
}

//----------------------------------------------------------------------------
void vtkPVWebInteractionEvent::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Buttons: " << this->Buttons << endl;
  os << indent << "Modifiers: " << this->Modifiers << endl;
  os << indent << "KeyCode: " << static_cast<int>(this->KeyCode) << endl;
  os << indent << "X: " << this->X << endl;
  os << indent << "Y: " << this->Y << endl;
}
