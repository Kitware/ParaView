/*=========================================================================

  Module:    vtkXMLPlaneWidgetWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPlaneWidgetWriter.h"

#include "vtkPlaneWidget.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLPropertyWriter.h"

vtkStandardNewMacro(vtkXMLPlaneWidgetWriter);
vtkCxxRevisionMacro(vtkXMLPlaneWidgetWriter, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLPlaneWidgetWriter::GetRootElementName()
{
  return "PlaneWidget";
}

//----------------------------------------------------------------------------
char* vtkXMLPlaneWidgetWriter::GetHandlePropertyElementName()
{
  return "HandleProperty";
}

//----------------------------------------------------------------------------
char* vtkXMLPlaneWidgetWriter::GetSelectedHandlePropertyElementName()
{
  return "SelectedHandleProperty";
}

//----------------------------------------------------------------------------
char* vtkXMLPlaneWidgetWriter::GetPlanePropertyElementName()
{
  return "PlaneProperty";
}

//----------------------------------------------------------------------------
char* vtkXMLPlaneWidgetWriter::GetSelectedPlanePropertyElementName()
{
  return "SelectedPlaneProperty";
}

//----------------------------------------------------------------------------
int vtkXMLPlaneWidgetWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkPlaneWidget *obj = vtkPlaneWidget::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The PlaneWidget is not set!");
    return 0;
    }

  elem->SetIntAttribute("Resolution", obj->GetResolution());

  elem->SetVectorAttribute("Origin", 3, obj->GetOrigin());

  elem->SetVectorAttribute("Point1", 3, obj->GetPoint1());

  elem->SetVectorAttribute("Point2", 3, obj->GetPoint2());

  elem->SetVectorAttribute("Center", 3, obj->GetCenter());

  elem->SetVectorAttribute("Normal", 3, obj->GetNormal());

  elem->SetIntAttribute("Representation", obj->GetRepresentation());

  elem->SetIntAttribute("NormalToXAxis", obj->GetNormalToXAxis());

  elem->SetIntAttribute("NormalToYAxis", obj->GetNormalToYAxis());

  elem->SetIntAttribute("NormalToZAxis", obj->GetNormalToZAxis());

  return 1;
}
//----------------------------------------------------------------------------
int vtkXMLPlaneWidgetWriter::AddNestedElements(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddNestedElements(elem))
    {
    return 0;
    }

  vtkPlaneWidget *obj = vtkPlaneWidget::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The PlaneWidget is not set!");
    return 0;
    }

  // Handle and Plane Property

  vtkXMLPropertyWriter *xmlw = vtkXMLPropertyWriter::New();
  vtkProperty *prop;

  prop = obj->GetHandleProperty();
  if (prop)
    {
    xmlw->SetObject(prop);
    xmlw->CreateInNestedElement(elem, this->GetHandlePropertyElementName());
    }
 
  prop = obj->GetSelectedHandleProperty();
  if (prop)
    {
    xmlw->SetObject(prop);
    xmlw->CreateInNestedElement(
      elem, this->GetSelectedHandlePropertyElementName());
    }
 
  prop = obj->GetPlaneProperty();
  if (prop)
    {
    xmlw->SetObject(prop);
    xmlw->CreateInNestedElement(elem, this->GetPlanePropertyElementName());
    }
 
  prop = obj->GetSelectedPlaneProperty();
  if (prop)
    {
    xmlw->SetObject(prop);
    xmlw->CreateInNestedElement(
      elem, this->GetSelectedPlanePropertyElementName());
    }

  xmlw->Delete();
 
  return 1;
}


