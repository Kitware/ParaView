/*=========================================================================

  Module:    vtkXMLActorWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLActorWriter.h"

#include "vtkActor.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLPropertyWriter.h"

vtkStandardNewMacro(vtkXMLActorWriter);
vtkCxxRevisionMacro(vtkXMLActorWriter, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLActorWriter::GetRootElementName()
{
  return "Actor";
}

//----------------------------------------------------------------------------
char* vtkXMLActorWriter::GetPropertyElementName()
{
  return "Property";
}

//----------------------------------------------------------------------------
char* vtkXMLActorWriter::GetBackfacePropertyElementName()
{
  return "BackfaceProperty";
}

//----------------------------------------------------------------------------
int vtkXMLActorWriter::AddNestedElements(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddNestedElements(elem))
    {
    return 0;
    }

  vtkActor *obj = vtkActor::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The Actor is not set!");
    return 0;
    }

  // Property

  vtkProperty *prop = obj->GetProperty();
  if (prop)
    {
    vtkXMLPropertyWriter *xmlw = vtkXMLPropertyWriter::New();
    xmlw->SetObject(prop);
    xmlw->CreateInNestedElement(elem, this->GetPropertyElementName());
    xmlw->Delete();
    }
 
  // Backface Property

  vtkProperty *bfprop = obj->GetBackfaceProperty();
  if (bfprop)
    {
    vtkXMLPropertyWriter *xmlw = vtkXMLPropertyWriter::New();
    xmlw->SetObject(bfprop);
    xmlw->CreateInNestedElement(elem, this->GetBackfacePropertyElementName());
    xmlw->Delete();
    }
 
  return 1;
}


