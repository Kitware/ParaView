/*=========================================================================

  Module:    vtkXMLTextActorWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLTextActorWriter.h"

#include "vtkObjectFactory.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLTextPropertyWriter.h"

vtkStandardNewMacro(vtkXMLTextActorWriter);
vtkCxxRevisionMacro(vtkXMLTextActorWriter, "1.2");

//----------------------------------------------------------------------------
char* vtkXMLTextActorWriter::GetRootElementName()
{
  return "TextActor";
}

//----------------------------------------------------------------------------
char* vtkXMLTextActorWriter::GetTextPropertyElementName()
{
  return "TextProperty";
}

//----------------------------------------------------------------------------
int vtkXMLTextActorWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkTextActor *obj = vtkTextActor::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The TextActor is not set!");
    return 0;
    }

  elem->SetAttribute("Input", obj->GetInput());

  elem->SetVectorAttribute("MinimumSize", 2, obj->GetMinimumSize());

  elem->SetFloatAttribute("MaximumLineHeight", obj->GetMaximumLineHeight());

  elem->SetIntAttribute("ScaledText", obj->GetScaledText());

  elem->SetIntAttribute("AlignmentPoint", obj->GetAlignmentPoint());

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLTextActorWriter::AddNestedElements(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddNestedElements(elem))
    {
    return 0;
    }

  vtkTextActor *obj = vtkTextActor::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The TextActor is not set!");
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


