/*=========================================================================

  Module:    vtkXMLKWRenderWidgetReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLKWRenderWidgetReader.h"

#include "vtkCamera.h"
#include "vtkCornerAnnotation.h"
#include "vtkKWRenderWidget.h"
#include "vtkObjectFactory.h"
#include "vtkTextActor.h"
#include "vtkXMLCameraReader.h"
#include "vtkXMLCornerAnnotationReader.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLKWRenderWidgetWriter.h"
#include "vtkXMLTextActorReader.h"

vtkStandardNewMacro(vtkXMLKWRenderWidgetReader);
vtkCxxRevisionMacro(vtkXMLKWRenderWidgetReader, "1.10");

//----------------------------------------------------------------------------
char* vtkXMLKWRenderWidgetReader::GetRootElementName()
{
  return "KWRenderWidget";
}

//----------------------------------------------------------------------------
int vtkXMLKWRenderWidgetReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkKWRenderWidget *obj = vtkKWRenderWidget::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The KWRenderWidget is not set!");
    return 0;
    }

  // Get attributes

  double dbuffer3[3];
  const char *cptr;

  if (elem->GetVectorAttribute("BackgroundColor", 3, dbuffer3) == 3)
    {
    obj->SetBackgroundColor(dbuffer3[0], dbuffer3[1], dbuffer3[2]);
    }

  cptr = elem->GetAttribute("DistanceUnits");
  if (cptr)
    {
    obj->SetDistanceUnits(cptr);
    }

  // Get nested elements

  // Camera

  vtkCamera *cam = obj->GetCurrentCamera();
  if (cam)
    {
    vtkXMLCameraReader *xmlr = vtkXMLCameraReader::New();
    xmlr->SetObject(cam);
    xmlr->ParseInNestedElement(
      elem, vtkXMLKWRenderWidgetWriter::GetCurrentCameraElementName());
    xmlr->Delete();
    }

  // Corner Annotation

  vtkCornerAnnotation *canno = obj->GetCornerAnnotation();
  if (canno)
    {
    vtkXMLCornerAnnotationReader *xmlr = vtkXMLCornerAnnotationReader::New();
    xmlr->SetObject(canno);
    if (xmlr->ParseInNestedElement(
          elem, vtkXMLKWRenderWidgetWriter::GetCornerAnnotationElementName()))
      {
      obj->SetCornerAnnotationVisibility(canno->GetVisibility()); // add prop
      }
    xmlr->Delete();
    }

  // Header Annotation

  vtkTextActor *texta = obj->GetHeaderAnnotation();
  if (texta)
    {
    vtkXMLTextActorReader *xmlr = vtkXMLTextActorReader::New();
    xmlr->SetObject(texta);
    if (xmlr->ParseInNestedElement(
          elem, vtkXMLKWRenderWidgetWriter::GetHeaderAnnotationElementName()))
      {
      obj->SetHeaderAnnotationVisibility(texta->GetVisibility()); // add prop
      }
    xmlr->Delete();
    }
  
  return 1;
}

