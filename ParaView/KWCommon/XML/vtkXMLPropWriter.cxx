/*=========================================================================

  Module:    vtkXMLPropWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPropWriter.h"

#include "vtkObjectFactory.h"
#include "vtkProp.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLPropWriter);
vtkCxxRevisionMacro(vtkXMLPropWriter, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLPropWriter::GetRootElementName()
{
  return "Prop";
}

//----------------------------------------------------------------------------
int vtkXMLPropWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkProp *obj = vtkProp::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The Prop is not set!");
    return 0;
    }

  elem->SetIntAttribute("Visibility", obj->GetVisibility());

  elem->SetIntAttribute("Pickable", obj->GetPickable());

  elem->SetIntAttribute("Dragable", obj->GetDragable());

  return 1;
}


