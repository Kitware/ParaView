/*=========================================================================

  Module:    vtkXML3DWidgetWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXML3DWidgetWriter.h"

#include "vtkObjectFactory.h"
#include "vtk3DWidget.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXML3DWidgetWriter);
vtkCxxRevisionMacro(vtkXML3DWidgetWriter, "1.3");

//----------------------------------------------------------------------------
char* vtkXML3DWidgetWriter::GetRootElementName()
{
  return "3DWidget";
}

//----------------------------------------------------------------------------
int vtkXML3DWidgetWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtk3DWidget *obj = vtk3DWidget::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The 3DWidget is not set!");
    return 0;
    }

  elem->SetFloatAttribute("PlaceFactor", obj->GetPlaceFactor());

  elem->SetFloatAttribute("HandleSize", obj->GetHandleSize());

  return 1;
}



