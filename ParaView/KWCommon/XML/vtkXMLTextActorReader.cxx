/*=========================================================================

  Module:    vtkXMLTextActorReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLTextActorReader.h"

#include "vtkObjectFactory.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLTextActorWriter.h"
#include "vtkXMLTextPropertyReader.h"

vtkStandardNewMacro(vtkXMLTextActorReader);
vtkCxxRevisionMacro(vtkXMLTextActorReader, "1.2");

//----------------------------------------------------------------------------
char* vtkXMLTextActorReader::GetRootElementName()
{
  return "TextActor";
}

//----------------------------------------------------------------------------
int vtkXMLTextActorReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkTextActor *obj = vtkTextActor::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The TextActor is not set!");
    return 0;
    }

  // Get attributes

  int ival, ibuffer2[2];
  float fval;
  const char *cptr;

  cptr = elem->GetAttribute("Input");
  if (cptr)
    {
    obj->SetInput(cptr);
    }
  
  if (elem->GetVectorAttribute("MinimumSize", 2, ibuffer2) == 2)
    {
    obj->SetMinimumSize(ibuffer2);
    }

  if (elem->GetScalarAttribute("MaximumLineHeight", fval))
    {
    obj->SetMaximumLineHeight(fval);
    }

  if (elem->GetScalarAttribute("ScaledText", ival))
    {
    obj->SetScaledText(ival);
    }

  if (elem->GetScalarAttribute("AlignmentPoint", ival))
    {
    obj->SetAlignmentPoint(ival);
    }
  
  // Get nested elements
  
  // Text property

  vtkXMLTextPropertyReader *xmlr = vtkXMLTextPropertyReader::New();

  if (xmlr->IsInNestedElement(
        elem, vtkXMLTextActorWriter::GetTextPropertyElementName()))
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
      elem, vtkXMLTextActorWriter::GetTextPropertyElementName());
    }

  xmlr->Delete();

  return 1;
}


