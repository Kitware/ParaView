/*=========================================================================

  Module:    vtkXMLCornerAnnotationWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLCornerAnnotationWriter.h"

#include "vtkCornerAnnotation.h"
#include "vtkObjectFactory.h"
#include "vtkTextProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLTextPropertyWriter.h"

vtkStandardNewMacro(vtkXMLCornerAnnotationWriter);
vtkCxxRevisionMacro(vtkXMLCornerAnnotationWriter, "1.4");

//----------------------------------------------------------------------------
char* vtkXMLCornerAnnotationWriter::GetRootElementName()
{
  return "CornerAnnotation";
}

//----------------------------------------------------------------------------
char* vtkXMLCornerAnnotationWriter::GetTextPropertyElementName()
{
  return "TextProperty";
}

//----------------------------------------------------------------------------
int vtkXMLCornerAnnotationWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkCornerAnnotation *obj = vtkCornerAnnotation::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The CornerAnnotation is not set!");
    return 0;
    }

  for (int i = 0; i < 4; i++)
    {
    ostrstream text_name;
    text_name << "Text" << i << ends;
    elem->SetAttribute(text_name.str(), obj->GetText(i));
    text_name.rdbuf()->freeze(0);
    }

  elem->SetFloatAttribute("MaximumLineHeight", obj->GetMaximumLineHeight());

  elem->SetIntAttribute("MinimumFontSize", obj->GetMinimumFontSize());

  elem->SetFloatAttribute("LevelShift", obj->GetLevelShift());

  elem->SetFloatAttribute("LevelScale", obj->GetLevelScale());

  elem->SetFloatAttribute("ShowSliceAndImage", obj->GetShowSliceAndImage());

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLCornerAnnotationWriter::AddNestedElements(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddNestedElements(elem))
    {
    return 0;
    }

  vtkCornerAnnotation *obj = vtkCornerAnnotation::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The CornerAnnotation is not set!");
    return 0;
    }

  // Text property

  vtkTextProperty *tprop = obj->GetTextProperty();
  if (tprop)
    {
    vtkXMLTextPropertyWriter *xmlw = vtkXMLTextPropertyWriter::New();
    xmlw->SetObject(tprop);
    xmlw->CreateInNestedElement(elem, this->GetTextPropertyElementName());
    xmlw->Delete();
    }
 
  return 1;
}

