/*=========================================================================

  Module:    vtkXMLImagePlaneWidgetWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLImagePlaneWidgetWriter.h"

#include "vtkImagePlaneWidget.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkTextProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLPropertyWriter.h"
#include "vtkXMLTextPropertyWriter.h"

vtkStandardNewMacro(vtkXMLImagePlaneWidgetWriter);
vtkCxxRevisionMacro(vtkXMLImagePlaneWidgetWriter, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLImagePlaneWidgetWriter::GetRootElementName()
{
  return "ImagePlaneWidget";
}

//----------------------------------------------------------------------------
char* vtkXMLImagePlaneWidgetWriter::GetPlanePropertyElementName()
{
  return "PlaneProperty";
}

//----------------------------------------------------------------------------
char* vtkXMLImagePlaneWidgetWriter::GetSelectedPlanePropertyElementName()
{
  return "SelectedPlaneProperty";
}

//----------------------------------------------------------------------------
char* vtkXMLImagePlaneWidgetWriter::GetCursorPropertyElementName()
{
  return "CursorProperty";
}

//----------------------------------------------------------------------------
char* vtkXMLImagePlaneWidgetWriter::GetMarginPropertyElementName()
{
  return "MarginProperty";
}

//----------------------------------------------------------------------------
char* vtkXMLImagePlaneWidgetWriter::GetTexturePlanePropertyElementName()
{
  return "TexturePlaneProperty";
}

//----------------------------------------------------------------------------
char* vtkXMLImagePlaneWidgetWriter::GetTextPropertyElementName()
{
  return "TextProperty";
}

//----------------------------------------------------------------------------
int vtkXMLImagePlaneWidgetWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkImagePlaneWidget *obj = vtkImagePlaneWidget::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The ImagePlaneWidget is not set!");
    return 0;
    }

  elem->SetVectorAttribute("Origin", 3, obj->GetOrigin());

  elem->SetVectorAttribute("Point1", 3, obj->GetPoint1());

  elem->SetVectorAttribute("Point2", 3, obj->GetPoint2());

  elem->SetIntAttribute("ResliceInterpolate", obj->GetResliceInterpolate());

  elem->SetIntAttribute(
    "RestrictPlaneToVolume", obj->GetRestrictPlaneToVolume());

  elem->SetIntAttribute("TextureInterpolate", obj->GetTextureInterpolate());

  elem->SetIntAttribute("TextureVisibility", obj->GetTextureVisibility());

  elem->SetIntAttribute("DisplayText", obj->GetDisplayText());

  return 1;
}
//----------------------------------------------------------------------------
int vtkXMLImagePlaneWidgetWriter::AddNestedElements(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddNestedElements(elem))
    {
    return 0;
    }

  vtkImagePlaneWidget *obj = vtkImagePlaneWidget::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The ImagePlaneWidget is not set!");
    return 0;
    }

  // Plane Properties

  vtkXMLPropertyWriter *xmlw = vtkXMLPropertyWriter::New();
  vtkProperty *prop;

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

  prop = obj->GetCursorProperty();
  if (prop)
    {
    xmlw->SetObject(prop);
    xmlw->CreateInNestedElement(elem, this->GetCursorPropertyElementName());
    }

  prop = obj->GetMarginProperty();
  if (prop)
    {
    xmlw->SetObject(prop);
    xmlw->CreateInNestedElement(elem, this->GetMarginPropertyElementName());
    }

  prop = obj->GetTexturePlaneProperty();
  if (prop)
    {
    xmlw->SetObject(prop);
    xmlw->CreateInNestedElement(
      elem, this->GetTexturePlanePropertyElementName());
    }

  xmlw->Delete();

  // Text properties

  vtkXMLTextPropertyWriter *xmltw = vtkXMLTextPropertyWriter::New();
  vtkTextProperty *tprop;

  tprop = obj->GetTextProperty();
  if (tprop)
    {
    xmltw->SetObject(tprop);
    xmltw->CreateInNestedElement(elem, this->GetTextPropertyElementName());
    }

  xmltw->Delete();

  return 1;
}


