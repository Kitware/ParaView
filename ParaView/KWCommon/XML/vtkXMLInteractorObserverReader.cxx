/*=========================================================================

  Module:    vtkXMLInteractorObserverReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLInteractorObserverReader.h"

#include "vtkObjectFactory.h"
#include "vtkInteractorObserver.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLInteractorObserverReader);
vtkCxxRevisionMacro(vtkXMLInteractorObserverReader, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLInteractorObserverReader::GetRootElementName()
{
  return "InteractorObserver";
}

//----------------------------------------------------------------------------
int vtkXMLInteractorObserverReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
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

  // Get attributes

  float fval;
  int ival;

  if (elem->GetScalarAttribute("Enabled", ival))
    {
    obj->SetEnabled(ival);
    }

  if (elem->GetScalarAttribute("Priority", fval))
    {
    obj->SetPriority(fval);
    }

  if (elem->GetScalarAttribute("KeyPressActivation", ival))
    {
    obj->SetKeyPressActivation(ival);
    }

  if (elem->GetScalarAttribute("KeyPressActivationValue", ival))
    {
    obj->SetKeyPressActivationValue(ival);
    }

  return 1;
}


