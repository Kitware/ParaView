/*=========================================================================

  Module:    vtkXMLPlaneWidgetReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPlaneWidgetReader.h"

#include "vtkPlaneWidget.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLPropertyReader.h"
#include "vtkXMLPlaneWidgetWriter.h"

vtkStandardNewMacro(vtkXMLPlaneWidgetReader);
vtkCxxRevisionMacro(vtkXMLPlaneWidgetReader, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLPlaneWidgetReader::GetRootElementName()
{
  return "PlaneWidget";
}

//----------------------------------------------------------------------------
int vtkXMLPlaneWidgetReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkPlaneWidget *obj = vtkPlaneWidget::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The PlaneWidget is not set!");
    return 0;
    }

  // Get attributes

  float fbuffer3[3];
  int ival;

  if (elem->GetScalarAttribute("Resolution", ival))
    {
    obj->SetResolution(ival);
    }

  if (elem->GetVectorAttribute("Origin", 3, fbuffer3) == 3)
    {
    obj->SetOrigin(fbuffer3);
    }

  if (elem->GetVectorAttribute("Point1", 3, fbuffer3) == 3)
    {
    obj->SetPoint1(fbuffer3);
    }

  if (elem->GetVectorAttribute("Point2", 3, fbuffer3) == 3)
    {
    obj->SetPoint2(fbuffer3);
    }

  if (elem->GetVectorAttribute("Center", 3, fbuffer3) == 3)
    {
    obj->SetCenter(fbuffer3);
    }

  if (elem->GetVectorAttribute("Normal", 3, fbuffer3) == 3)
    {
    obj->SetNormal(fbuffer3);
    }

  if (elem->GetScalarAttribute("Representation", ival))
    {
    obj->SetRepresentation(ival);
    }

  if (elem->GetScalarAttribute("NormalToXAxis", ival))
    {
    obj->SetNormalToXAxis(ival);
    }

  if (elem->GetScalarAttribute("NormalToYAxis", ival))
    {
    obj->SetNormalToYAxis(ival);
    }

  if (elem->GetScalarAttribute("NormalToZAxis", ival))
    {
    obj->SetNormalToZAxis(ival);
    }

  // Get nested elements
  
  // Handle and plane property

  vtkXMLPropertyReader *xmlr = vtkXMLPropertyReader::New();
  vtkProperty *prop;

  prop = obj->GetHandleProperty();
  if (prop)
    {
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLPlaneWidgetWriter::GetHandlePropertyElementName());
    }

  prop = obj->GetSelectedHandleProperty();
  if (prop)
    {
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLPlaneWidgetWriter::GetSelectedHandlePropertyElementName());
    }

  if (xmlr->IsInNestedElement(
        elem, vtkXMLPlaneWidgetWriter::GetPlanePropertyElementName()))
    {
    prop = obj->GetPlaneProperty();
    if (!prop)
      {
      prop = vtkProperty::New();
      obj->SetPlaneProperty(prop);
      prop->Delete();
      }
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLPlaneWidgetWriter::GetPlanePropertyElementName());
    }

  prop = obj->GetSelectedPlaneProperty();
  if (prop)
    {
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLPlaneWidgetWriter::GetSelectedPlanePropertyElementName());
    }

  xmlr->Delete();

  return 1;
}


