/*=========================================================================

  Module:    vtkXMLCornerAnnotationReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLCornerAnnotationReader.h"

#include "vtkCornerAnnotation.h"
#include "vtkObjectFactory.h"
#include "vtkTextProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLTextPropertyReader.h"
#include "vtkXMLCornerAnnotationWriter.h"

vtkStandardNewMacro(vtkXMLCornerAnnotationReader);
vtkCxxRevisionMacro(vtkXMLCornerAnnotationReader, "1.4");

//----------------------------------------------------------------------------
char* vtkXMLCornerAnnotationReader::GetRootElementName()
{
  return "CornerAnnotation";
}

//----------------------------------------------------------------------------
int vtkXMLCornerAnnotationReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkCornerAnnotation *obj = vtkCornerAnnotation::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The CornerAnnotation is not set!");
    return 0;
    }

  // Get attributes

  float fval;
  int ival;
  const char *cptr;

  for (int i = 0; i < 4; i++)
    {
    ostrstream text_name;
    text_name << "Text" << i << ends;
    cptr = elem->GetAttribute(text_name.str());
    if (cptr)
      {
      obj->SetText(i, cptr);
      }
    text_name.rdbuf()->freeze(0);
    }

  if (elem->GetScalarAttribute("MaximumLineHeight", fval))
    {
    obj->SetMaximumLineHeight(fval);
    }

  if (elem->GetScalarAttribute("MinimumFontSize", ival))
    {
    obj->SetMinimumFontSize(ival);
    }

  if (elem->GetScalarAttribute("LevelShift", fval))
    {
    obj->SetLevelShift(fval);
    }

  if (elem->GetScalarAttribute("LevelScale", fval))
    {
    obj->SetLevelScale(fval);
    }

  if (elem->GetScalarAttribute("ShowSliceAndImage", ival))
    {
    obj->SetShowSliceAndImage(ival);
    }

  // Get nested elements
  
  // Text property

  vtkXMLTextPropertyReader *xmlr = vtkXMLTextPropertyReader::New();
  if (xmlr->IsInNestedElement(
        elem, vtkXMLCornerAnnotationWriter::GetTextPropertyElementName()))
    {
    vtkTextProperty *tprop = obj->GetTextProperty();
    if (!tprop)
      {
      tprop = vtkTextProperty::New();
      obj->SetTextProperty(tprop);
      tprop->Delete();
      }
    xmlr->SetObject(tprop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLCornerAnnotationWriter::GetTextPropertyElementName());
    }
  xmlr->Delete();
  
  return 1;
}

