/*=========================================================================

  Module:    vtkXMLActor2DReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLActor2DReader.h"

#include "vtkActor2D.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLProperty2DReader.h"
#include "vtkXMLActor2DWriter.h"

vtkStandardNewMacro(vtkXMLActor2DReader);
vtkCxxRevisionMacro(vtkXMLActor2DReader, "1.4");

//----------------------------------------------------------------------------
char* vtkXMLActor2DReader::GetRootElementName()
{
  return "Actor2D";
}

//----------------------------------------------------------------------------
int vtkXMLActor2DReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkActor2D *obj = vtkActor2D::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The Actor2D is not set!");
    return 0;
    }

  // Get attributes

  float fbuffer2[2];
  int ival;

  if (elem->GetScalarAttribute("LayerNumber", ival))
    {
    obj->SetLayerNumber(ival);
    }

  vtkCoordinate *coord = obj->GetPositionCoordinate();
  if (coord && elem->GetVectorAttribute("Position", 2, fbuffer2) == 2)
    {
    int sys = coord->GetCoordinateSystem();
    coord->SetCoordinateSystemToNormalizedViewport();
    coord->SetValue(fbuffer2[0], fbuffer2[1]);
    coord->SetCoordinateSystem(sys);
    }
  
  coord = obj->GetPosition2Coordinate();
  if (coord && elem->GetVectorAttribute("Position2", 2, fbuffer2) == 2)
    {
    int sys = coord->GetCoordinateSystem();
    coord->SetCoordinateSystemToNormalizedViewport();
    coord->SetValue(fbuffer2[0], fbuffer2[1]);
    coord->SetCoordinateSystem(sys);
    }
  
  // Get nested elements
  
  // Property 2D

  vtkXMLProperty2DReader *xmlr = vtkXMLProperty2DReader::New();
  if (xmlr->IsInNestedElement(
        elem, vtkXMLActor2DWriter::GetPropertyElementName()))
    {
    vtkProperty2D *prop2d = obj->GetProperty();
    if (!prop2d)
      {
      prop2d = vtkProperty2D::New();
      obj->SetProperty(prop2d);
      prop2d->Delete();
      }
    xmlr->SetObject(prop2d);
    xmlr->ParseInNestedElement(
      elem, vtkXMLActor2DWriter::GetPropertyElementName());
    }
  xmlr->Delete();
  
  return 1;
}



