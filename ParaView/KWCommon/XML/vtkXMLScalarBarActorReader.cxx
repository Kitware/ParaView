/*=========================================================================

  Module:    vtkXMLScalarBarActorReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLScalarBarActorReader.h"

#include "vtkObjectFactory.h"
#include "vtkScalarBarActor.h"
#include "vtkTextProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLScalarBarActorWriter.h"
#include "vtkXMLTextPropertyReader.h"

vtkStandardNewMacro(vtkXMLScalarBarActorReader);
vtkCxxRevisionMacro(vtkXMLScalarBarActorReader, "1.4");

//----------------------------------------------------------------------------
char* vtkXMLScalarBarActorReader::GetRootElementName()
{
  return "ScalarBarActor";
}

//----------------------------------------------------------------------------
int vtkXMLScalarBarActorReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkScalarBarActor *obj = vtkScalarBarActor::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The ScalarBarActor is not set!");
    return 0;
    }

  // Get attributes

  int ival;
  const char *cptr;

  if (elem->GetScalarAttribute("MaximumNumberOfColors", ival))
    {
    obj->SetMaximumNumberOfColors(ival);
    }

  if (elem->GetScalarAttribute("NumberOfLabels", ival))
    {
    obj->SetNumberOfLabels(ival);
    }

  if (elem->GetScalarAttribute("Orientation", ival))
    {
    obj->SetOrientation(ival);
    }
  
  cptr = elem->GetAttribute("LabelFormat");
  if (cptr)
    {
    obj->SetLabelFormat(cptr);
    }

  cptr = elem->GetAttribute("Title");
  if (cptr)
    {
    obj->SetTitle(cptr);
    }
  
  // Get nested elements
  
  // Title and label text property

  vtkXMLTextPropertyReader *xmlr = vtkXMLTextPropertyReader::New();

  if (xmlr->IsInNestedElement(
        elem, vtkXMLScalarBarActorWriter::GetTitleTextPropertyElementName()))
    {
    vtkTextProperty *tprop = obj->GetTitleTextProperty();
    if (!tprop)
      {
      tprop = vtkTextProperty::New();
      obj->SetTitleTextProperty(tprop);
      tprop->Delete();
      }
    xmlr->SetObject(tprop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLScalarBarActorWriter::GetTitleTextPropertyElementName());
    }

  if (xmlr->IsInNestedElement(
        elem, vtkXMLScalarBarActorWriter::GetLabelTextPropertyElementName()))
    {
    vtkTextProperty *tprop = obj->GetLabelTextProperty();
    if (!tprop)
      {
      tprop = vtkTextProperty::New();
      obj->SetLabelTextProperty(tprop);
      tprop->Delete();
      }
    xmlr->SetObject(tprop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLScalarBarActorWriter::GetLabelTextPropertyElementName());
    }

  xmlr->Delete();

  return 1;
}


