/*=========================================================================

  Module:    vtkXMLKWSelectionFrameReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLKWSelectionFrameReader.h"

#include "vtkObjectFactory.h"
#include "vtkKWSelectionFrame.h"
#include "vtkXMLKWSelectionFrameWriter.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLKWSelectionFrameReader);
vtkCxxRevisionMacro(vtkXMLKWSelectionFrameReader, "1.1");

//----------------------------------------------------------------------------
char* vtkXMLKWSelectionFrameReader::GetRootElementName()
{
  return "KWSelectionFrame";
}

//----------------------------------------------------------------------------
int vtkXMLKWSelectionFrameReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkKWSelectionFrame *obj = vtkKWSelectionFrame::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The KWSelectionFrame is not set!");
    return 0;
    }

  // Get attributes

  float fbuffer3[3];
  const char *cptr;
  int ival;

  cptr = elem->GetAttribute("Title");
  if (cptr)
    {
    obj->SetTitle(cptr);
    }

  if (elem->GetScalarAttribute("Selected", ival))
    {
    obj->SetSelected(ival);
    }

  if (elem->GetScalarAttribute("ShowSelectionList", ival))
    {
    obj->SetShowSelectionList(ival);
    }

  if (elem->GetVectorAttribute("TitleColor", 3, fbuffer3) == 3)
    {
    obj->SetTitleColor(fbuffer3);
    }

  if (elem->GetVectorAttribute("TitleSelectedColor", 3, fbuffer3) == 3)
    {
    obj->SetTitleSelectedColor(fbuffer3);
    }
  
  if (elem->GetVectorAttribute("TitleBackgroundColor", 3, fbuffer3) == 3)
    {
    obj->SetTitleBackgroundColor(fbuffer3);
    }
  
  if (elem->GetVectorAttribute("TitleBackgroundSelectedColor",3,fbuffer3) == 3)
    {
    obj->SetTitleBackgroundSelectedColor(fbuffer3);
    }
  
  if (elem->GetScalarAttribute("ShowToolbarSet", ival))
    {
    obj->SetShowToolbarSet(ival);
    }

  return 1;
}

