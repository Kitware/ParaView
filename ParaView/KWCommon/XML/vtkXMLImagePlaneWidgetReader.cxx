/*=========================================================================

  Module:    vtkXMLImagePlaneWidgetReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLImagePlaneWidgetReader.h"

#include "vtkImagePlaneWidget.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkTextProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLImagePlaneWidgetWriter.h"
#include "vtkXMLPropertyReader.h"
#include "vtkXMLTextPropertyReader.h"

vtkStandardNewMacro(vtkXMLImagePlaneWidgetReader);
vtkCxxRevisionMacro(vtkXMLImagePlaneWidgetReader, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLImagePlaneWidgetReader::GetRootElementName()
{
  return "ImagePlaneWidget";
}

//----------------------------------------------------------------------------
int vtkXMLImagePlaneWidgetReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkImagePlaneWidget *obj = vtkImagePlaneWidget::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The ImagePlaneWidget is not set!");
    return 0;
    }

  // Get attributes

  float fbuffer3[3];
  int ival;

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

  if (elem->GetScalarAttribute("ResliceInterpolate", ival))
    {
    obj->SetResliceInterpolate(ival);
    }

  if (elem->GetScalarAttribute("RestrictPlaneToVolume", ival))
    {
    obj->SetRestrictPlaneToVolume(ival);
    }

  if (elem->GetScalarAttribute("TextureInterpolate", ival))
    {
    obj->SetTextureInterpolate(ival);
    }

  if (elem->GetScalarAttribute("TextureVisibility", ival))
    {
    obj->SetTextureVisibility(ival);
    }

  if (elem->GetScalarAttribute("DisplayText", ival))
    {
    obj->SetDisplayText(ival);
    }

  // Get nested elements
  
  // Plane properties

  vtkXMLPropertyReader *xmlr = vtkXMLPropertyReader::New();
  vtkProperty *prop;

  prop = obj->GetPlaneProperty();
  if (prop)
    {
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLImagePlaneWidgetWriter::GetPlanePropertyElementName());
    }

  prop = obj->GetSelectedPlaneProperty();
  if (prop)
    {
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, 
      vtkXMLImagePlaneWidgetWriter::GetSelectedPlanePropertyElementName());
    }

  prop = obj->GetCursorProperty();
  if (prop)
    {
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLImagePlaneWidgetWriter::GetCursorPropertyElementName());
    }

  prop = obj->GetMarginProperty();
  if (prop)
    {
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLImagePlaneWidgetWriter::GetMarginPropertyElementName());
    }

  prop = obj->GetTexturePlaneProperty();
  if (prop)
    {
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLImagePlaneWidgetWriter::GetTexturePlanePropertyElementName());
    }

  xmlr->Delete();

  // Text properties

  vtkXMLTextPropertyReader *xmltr = vtkXMLTextPropertyReader::New();
  vtkTextProperty *tprop;

  tprop = obj->GetTextProperty();
  if (tprop)
    {
    xmltr->SetObject(tprop);
    xmltr->ParseInNestedElement(
      elem, vtkXMLImagePlaneWidgetWriter::GetTextPropertyElementName());
    }

  xmltr->Delete();

  return 1;
}


