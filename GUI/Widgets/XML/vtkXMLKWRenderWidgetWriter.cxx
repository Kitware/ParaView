/*=========================================================================

  Module:    vtkXMLKWRenderWidgetWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLKWRenderWidgetWriter.h"

#include "vtkCamera.h"
#include "vtkCornerAnnotation.h"
#include "vtkKWRenderWidget.h"
#include "vtkObjectFactory.h"
#include "vtkTextActor.h"
#include "vtkXMLCameraWriter.h"
#include "vtkXMLCornerAnnotationWriter.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLTextActorWriter.h"

vtkStandardNewMacro(vtkXMLKWRenderWidgetWriter);
vtkCxxRevisionMacro(vtkXMLKWRenderWidgetWriter, "1.10");

//----------------------------------------------------------------------------
char* vtkXMLKWRenderWidgetWriter::GetRootElementName()
{
  return "KWRenderWidget";
}

//----------------------------------------------------------------------------
char* vtkXMLKWRenderWidgetWriter::GetCurrentCameraElementName()
{
  return "CurrentCamera";
}

//----------------------------------------------------------------------------
char* vtkXMLKWRenderWidgetWriter::GetCornerAnnotationElementName()
{
  return "CornerAnnotation";
}

//----------------------------------------------------------------------------
char* vtkXMLKWRenderWidgetWriter::GetHeaderAnnotationElementName()
{
  return "HeaderAnnotation";
}

//----------------------------------------------------------------------------
int vtkXMLKWRenderWidgetWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkKWRenderWidget *obj = vtkKWRenderWidget::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The KWRenderWidget is not set!");
    return 0;
    }

  double rgb[3];
  obj->GetRendererBackgroundColor(rgb, rgb + 1, rgb + 2);
  elem->SetVectorAttribute("RendererBackgroundColor", 3, rgb);

  elem->SetAttribute("DistanceUnits", obj->GetDistanceUnits());

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLKWRenderWidgetWriter::AddNestedElements(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddNestedElements(elem))
    {
    return 0;
    }

  vtkKWRenderWidget *obj = vtkKWRenderWidget::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The KWRenderWidget is not set!");
    return 0;
    }

  // Camera

  vtkCamera *cam = obj->GetCurrentCamera();
  if (cam)
    {
    vtkXMLCameraWriter *xmlw = vtkXMLCameraWriter::New();
    xmlw->SetObject(cam);
    xmlw->CreateInNestedElement(elem, this->GetCurrentCameraElementName());
    xmlw->Delete();
    }

  // Corner Annotation

  vtkCornerAnnotation *canno = obj->GetCornerAnnotation();
  if (canno)
    {
    vtkXMLCornerAnnotationWriter *xmlw = vtkXMLCornerAnnotationWriter::New();
    xmlw->SetObject(canno);
    xmlw->CreateInNestedElement(elem, this->GetCornerAnnotationElementName());
    xmlw->Delete();
    }

  // Header Annotation

  vtkTextActor *texta = obj->GetHeaderAnnotation();
  if (texta)
    {
    vtkXMLTextActorWriter *xmlw = vtkXMLTextActorWriter::New();
    xmlw->SetObject(texta);
    xmlw->CreateInNestedElement(elem, this->GetHeaderAnnotationElementName());
    xmlw->Delete();
    }
  
  return 1;
}

