/*=========================================================================

  Module:    vtkXMLInteractorObserverWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLInteractorObserverWriter.h"

#include "vtkObjectFactory.h"
#include "vtkInteractorObserver.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLInteractorObserverWriter);
vtkCxxRevisionMacro(vtkXMLInteractorObserverWriter, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLInteractorObserverWriter::GetRootElementName()
{
  return "InteractorObserver";
}

//----------------------------------------------------------------------------
int vtkXMLInteractorObserverWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkInteractorObserver *obj = 
    vtkInteractorObserver::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The InteractorObserver is not set!");
    return 0;
    }

  elem->SetIntAttribute("Enabled", obj->GetEnabled());

  elem->SetFloatAttribute("Priority", obj->GetPriority());

  elem->SetIntAttribute("KeyPressActivation", obj->GetKeyPressActivation());

  elem->SetIntAttribute("KeyPressActivationValue", 
                        obj->GetKeyPressActivationValue());

  return 1;
}


