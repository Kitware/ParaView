/*=========================================================================

  Module:    vtkXML3DWidgetReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXML3DWidgetReader.h"

#include "vtkObjectFactory.h"
#include "vtk3DWidget.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXML3DWidgetReader);
vtkCxxRevisionMacro(vtkXML3DWidgetReader, "1.3");

//----------------------------------------------------------------------------
char* vtkXML3DWidgetReader::GetRootElementName()
{
  return "3DWidget";
}

//----------------------------------------------------------------------------
int vtkXML3DWidgetReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtk3DWidget *obj = vtk3DWidget::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The 3DWidget is not set!");
    return 0;
    }

  // Get attributes

  float fval;

  if (elem->GetScalarAttribute("PlaceFactor", fval))
    {
    obj->SetPlaceFactor(fval);
    }

  if (elem->GetScalarAttribute("HandleSize", fval))
    {
    obj->SetHandleSize(fval);
    }

  return 1;
}



