/*=========================================================================

  Module:    vtkXMLPropReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPropReader.h"

#include "vtkObjectFactory.h"
#include "vtkProp.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLPropReader);
vtkCxxRevisionMacro(vtkXMLPropReader, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLPropReader::GetRootElementName()
{
  return "Prop";
}

//----------------------------------------------------------------------------
int vtkXMLPropReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkProp *obj = vtkProp::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The Prop is not set!");
    return 0;
    }

  // Get attributes

  int ival;

  if (elem->GetScalarAttribute("Visibility", ival))
    {
    obj->SetVisibility(ival);
    }

  if (elem->GetScalarAttribute("Pickable", ival))
    {
    obj->SetPickable(ival);
    }

  if (elem->GetScalarAttribute("Dragable", ival))
    {
    obj->SetDragable(ival);
    }
  
  return 1;
}


