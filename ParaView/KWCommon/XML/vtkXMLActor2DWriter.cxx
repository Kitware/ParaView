/*=========================================================================

  Module:    vtkXMLActor2DWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLActor2DWriter.h"

#include "vtkActor2D.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLProperty2DWriter.h"

vtkStandardNewMacro(vtkXMLActor2DWriter);
vtkCxxRevisionMacro(vtkXMLActor2DWriter, "1.6");

//----------------------------------------------------------------------------
char* vtkXMLActor2DWriter::GetRootElementName()
{
  return "Actor2D";
}

//----------------------------------------------------------------------------
char* vtkXMLActor2DWriter::GetPropertyElementName()
{
  return "Property";
}

//----------------------------------------------------------------------------
int vtkXMLActor2DWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkActor2D *obj = vtkActor2D::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The Actor2D is not set!");
    return 0;
    }

  elem->SetIntAttribute("LayerNumber", obj->GetLayerNumber());

  vtkCoordinate *coord = obj->GetPositionCoordinate();
  if (coord)
    {
    int sys = coord->GetCoordinateSystem();
    coord->SetCoordinateSystemToNormalizedViewport();
    elem->SetVectorAttribute("Position", 2, coord->GetValue());
    coord->SetCoordinateSystem(sys);
    }

  coord = obj->GetPosition2Coordinate();
  if (coord)
    {
    int sys = coord->GetCoordinateSystem();
    coord->SetCoordinateSystemToNormalizedViewport();
    elem->SetVectorAttribute("Position2", 2, coord->GetValue());
    coord->SetCoordinateSystem(sys);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLActor2DWriter::AddNestedElements(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddNestedElements(elem))
    {
    return 0;
    }

  vtkActor2D *obj = vtkActor2D::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The Actor2D is not set!");
    return 0;
    }

  // Property2D

  vtkProperty2D *prop2d = obj->GetProperty();
  if (prop2d)
    {
    vtkXMLProperty2DWriter *xmlw = vtkXMLProperty2DWriter::New();
    xmlw->SetObject(prop2d);
    xmlw->CreateInNestedElement(elem, this->GetPropertyElementName());
    xmlw->Delete();
    }
 
  return 1;
}
