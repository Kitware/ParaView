/*=========================================================================

  Module:    vtkXMLProperty2DWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLProperty2DWriter.h"

#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLProperty2DWriter);
vtkCxxRevisionMacro(vtkXMLProperty2DWriter, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLProperty2DWriter::GetRootElementName()
{
  return "Property2D";
}

//----------------------------------------------------------------------------
int vtkXMLProperty2DWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkProperty2D *obj = vtkProperty2D::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The Property2D is not set!");
    return 0;
    }

  elem->SetVectorAttribute("Color", 3, obj->GetColor());

  elem->SetFloatAttribute("Opacity", obj->GetOpacity());

  elem->SetFloatAttribute("PointSize", obj->GetPointSize());
  
  elem->SetFloatAttribute("LineWidth", obj->GetLineWidth());

  elem->SetIntAttribute("LineStipplePattern", obj->GetLineStipplePattern());

  elem->SetIntAttribute("LineStippleRepeatFactor", 
                        obj->GetLineStippleRepeatFactor());
  
  elem->SetIntAttribute("DisplayLocation", obj->GetDisplayLocation());

  return 1;
}


