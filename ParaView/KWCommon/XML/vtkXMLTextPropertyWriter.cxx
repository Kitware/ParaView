/*=========================================================================

  Module:    vtkXMLTextPropertyWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLTextPropertyWriter.h"

#include "vtkObjectFactory.h"
#include "vtkTextProperty.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLTextPropertyWriter);
vtkCxxRevisionMacro(vtkXMLTextPropertyWriter, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLTextPropertyWriter::GetRootElementName()
{
  return "TextProperty";
}

//----------------------------------------------------------------------------
int vtkXMLTextPropertyWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkTextProperty *obj = vtkTextProperty::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The TextProperty is not set!");
    return 0;
    }

  elem->SetVectorAttribute("Color", 3, obj->GetColor());

  elem->SetFloatAttribute("Opacity", obj->GetOpacity());

  elem->SetIntAttribute("FontFamily", obj->GetFontFamily());

  elem->SetIntAttribute("FontSize", obj->GetFontSize());

  elem->SetIntAttribute("Bold", obj->GetBold());

  elem->SetIntAttribute("Italic", obj->GetItalic());

  elem->SetIntAttribute("Shadow", obj->GetShadow());

  elem->SetIntAttribute("AntiAliasing", obj->GetAntiAliasing());

  elem->SetIntAttribute("Justification", obj->GetJustification());

  elem->SetIntAttribute("VerticalJustification", 
                        obj->GetVerticalJustification());

  elem->SetFloatAttribute("LineOffset", obj->GetLineOffset());

  elem->SetFloatAttribute("LineSpacing", obj->GetLineSpacing());

  return 1;
}


