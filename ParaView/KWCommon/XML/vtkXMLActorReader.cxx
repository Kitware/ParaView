/*=========================================================================

  Module:    vtkXMLActorReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLActorReader.h"

#include "vtkActor.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLPropertyReader.h"
#include "vtkXMLActorWriter.h"

vtkStandardNewMacro(vtkXMLActorReader);
vtkCxxRevisionMacro(vtkXMLActorReader, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLActorReader::GetRootElementName()
{
  return "Actor";
}

//----------------------------------------------------------------------------
int vtkXMLActorReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkActor *obj = vtkActor::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The Actor is not set!");
    return 0;
    }

  // Get nested elements
  
  // Property and backface property

  vtkXMLPropertyReader *xmlr = vtkXMLPropertyReader::New();

  if (xmlr->IsInNestedElement(
        elem, vtkXMLActorWriter::GetPropertyElementName()))
    {
    vtkProperty *prop = obj->GetProperty();
    if (!prop)
      {
      prop = vtkProperty::New();
      obj->SetProperty(prop);
      prop->Delete();
      }
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLActorWriter::GetPropertyElementName());
    }

  if (xmlr->IsInNestedElement(
        elem, vtkXMLActorWriter::GetBackfacePropertyElementName()))
    {
    vtkProperty *prop = obj->GetBackfaceProperty();
    if (!prop)
      {
      prop = vtkProperty::New();
      obj->SetBackfaceProperty(prop);
      prop->Delete();
      }
    xmlr->SetObject(prop);
    xmlr->ParseInNestedElement(
      elem, vtkXMLActorWriter::GetBackfacePropertyElementName());
    }

  xmlr->Delete();

  return 1;
}



