/*=========================================================================

  Module:    vtkXMLKWSelectionFrameWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLKWSelectionFrameWriter.h"

#include "vtkObjectFactory.h"
#include "vtkKWSelectionFrame.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLKWSelectionFrameWriter);
vtkCxxRevisionMacro(vtkXMLKWSelectionFrameWriter, "1.1");

//----------------------------------------------------------------------------
char* vtkXMLKWSelectionFrameWriter::GetRootElementName()
{
  return "KWSelectionFrame";
}

//----------------------------------------------------------------------------
int vtkXMLKWSelectionFrameWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkKWSelectionFrame *obj = vtkKWSelectionFrame::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The KWSelectionFrame is not set!");
    return 0;
    }

  elem->SetAttribute("Title", obj->GetTitle());

  elem->SetIntAttribute("Selected", obj->GetSelected());

  elem->SetIntAttribute("ShowSelectionList", obj->GetShowSelectionList());

  elem->SetVectorAttribute(
    "TitleColor", 3, obj->GetTitleColor());

  elem->SetVectorAttribute(
    "TitleSelectedColor", 3, obj->GetTitleSelectedColor());

  elem->SetVectorAttribute(
    "TitleBackgroundColor", 3, obj->GetTitleBackgroundColor());

  elem->SetVectorAttribute(
    "TitleBackgroundSelectedColor", 3,obj->GetTitleBackgroundSelectedColor());

  elem->SetIntAttribute("ShowToolbarSet", obj->GetShowToolbarSet());

  return 1;
}
