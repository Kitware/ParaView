/*=========================================================================

  Module:    vtkXMLCameraWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLCameraWriter.h"

#include "vtkObjectFactory.h"
#include "vtkCamera.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLCameraWriter);
vtkCxxRevisionMacro(vtkXMLCameraWriter, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLCameraWriter::GetRootElementName()
{
  return "Camera";
}

//----------------------------------------------------------------------------
int vtkXMLCameraWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkCamera *obj = vtkCamera::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The Camera is not set!");
    return 0;
    }

  elem->SetIntAttribute("ParallelProjection", obj->GetParallelProjection());

  elem->SetVectorAttribute("Position", 3, obj->GetPosition());

  elem->SetVectorAttribute("FocalPoint", 3, obj->GetFocalPoint());

  elem->SetVectorAttribute("ViewUp", 3, obj->GetViewUp());

  elem->SetVectorAttribute("ClippingRange", 3, obj->GetClippingRange());

  elem->SetDoubleAttribute("ViewAngle", obj->GetViewAngle());

  elem->SetDoubleAttribute("ParallelScale", obj->GetParallelScale());

  return 1;
}


