/*=========================================================================

  Module:    vtkXMLScalarBarActorWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLScalarBarActorWriter.h"

#include "vtkObjectFactory.h"
#include "vtkScalarBarActor.h"
#include "vtkTextProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLTextPropertyWriter.h"

vtkStandardNewMacro(vtkXMLScalarBarActorWriter);
vtkCxxRevisionMacro(vtkXMLScalarBarActorWriter, "1.4");

//----------------------------------------------------------------------------
char* vtkXMLScalarBarActorWriter::GetRootElementName()
{
  return "ScalarBarActor";
}

//----------------------------------------------------------------------------
char* vtkXMLScalarBarActorWriter::GetTitleTextPropertyElementName()
{
  return "TitleTextProperty";
}

//----------------------------------------------------------------------------
char* vtkXMLScalarBarActorWriter::GetLabelTextPropertyElementName()
{
  return "LabelTextProperty";
}

//----------------------------------------------------------------------------
int vtkXMLScalarBarActorWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkScalarBarActor *obj = vtkScalarBarActor::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The ScalarBarActor is not set!");
    return 0;
    }

  elem->SetIntAttribute(
    "MaximumNumberOfColors", obj->GetMaximumNumberOfColors());

  elem->SetIntAttribute("NumberOfLabels", obj->GetNumberOfLabels());

  elem->SetIntAttribute("Orientation", obj->GetOrientation());

  elem->SetAttribute("LabelFormat", obj->GetLabelFormat());

  elem->SetAttribute("Title", obj->GetTitle());

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLScalarBarActorWriter::AddNestedElements(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddNestedElements(elem))
    {
    return 0;
    }

  vtkScalarBarActor *obj = vtkScalarBarActor::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The ScalarBarActor is not set!");
    return 0;
    }

  // Title text property

  vtkTextProperty *tprop = obj->GetTitleTextProperty();
  if (tprop)
    {
    vtkXMLTextPropertyWriter *xmlw = vtkXMLTextPropertyWriter::New();
    xmlw->SetObject(tprop);
    xmlw->CreateInNestedElement(elem, this->GetTitleTextPropertyElementName());
    xmlw->Delete();
    }
 
  // Label text property

  vtkTextProperty *lprop = obj->GetLabelTextProperty();
  if (lprop)
    {
    vtkXMLTextPropertyWriter *xmlw = vtkXMLTextPropertyWriter::New();
    xmlw->SetObject(lprop);
    xmlw->CreateInNestedElement(elem, this->GetLabelTextPropertyElementName());
    xmlw->Delete();
    }
 
  return 1;
}


