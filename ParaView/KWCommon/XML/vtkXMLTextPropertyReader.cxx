/*=========================================================================

  Module:    vtkXMLTextPropertyReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLTextPropertyReader.h"

#include "vtkObjectFactory.h"
#include "vtkTextProperty.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLTextPropertyReader);
vtkCxxRevisionMacro(vtkXMLTextPropertyReader, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLTextPropertyReader::GetRootElementName()
{
  return "TextProperty";
}

//----------------------------------------------------------------------------
int vtkXMLTextPropertyReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkTextProperty *obj = vtkTextProperty::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The TextProperty is not set!");
    return 0;
    }

  // Get attributes

  float fbuffer3[3], fval;
  int ival;

  if (elem->GetVectorAttribute("Color", 3, fbuffer3) == 3)
    {
    obj->SetColor(fbuffer3);
    }

  if (elem->GetScalarAttribute("Opacity", fval))
    {
    obj->SetOpacity(fval);
    }

  if (elem->GetScalarAttribute("FontFamily", ival))
    {
    obj->SetFontFamily(ival);
    }

  if (elem->GetScalarAttribute("FontSize", ival))
    {
    obj->SetFontSize(ival);
    }

  if (elem->GetScalarAttribute("Bold", ival))
    {
    obj->SetBold(ival);
    }

  if (elem->GetScalarAttribute("Italic", ival))
    {
    obj->SetItalic(ival);
    }

  if (elem->GetScalarAttribute("Shadow", ival))
    {
    obj->SetShadow(ival);
    }

  if (elem->GetScalarAttribute("AntiAliasing", ival))
    {
    obj->SetAntiAliasing(ival);
    }

  if (elem->GetScalarAttribute("Justification", ival))
    {
    obj->SetJustification(ival);
    }

  if (elem->GetScalarAttribute("VerticalJustification", ival))
    {
    obj->SetVerticalJustification(ival);
    }

  if (elem->GetScalarAttribute("LineOffset", fval))
    {
    obj->SetLineOffset(fval);
    }

  if (elem->GetScalarAttribute("LineSpacing", fval))
    {
    obj->SetLineSpacing(fval);
    }

  return 1;
}


