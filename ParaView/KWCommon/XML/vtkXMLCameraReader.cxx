/*=========================================================================

  Module:    vtkXMLCameraReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLCameraReader.h"

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLCameraReader);
vtkCxxRevisionMacro(vtkXMLCameraReader, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLCameraReader::GetRootElementName()
{
  return "Camera";
}

//----------------------------------------------------------------------------
int vtkXMLCameraReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkCamera *obj = vtkCamera::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The Camera is not set!");
    return 0;
    }

  // Get attributes

  double dbuffer3[3], dval;
  int ival;

  if (elem->GetScalarAttribute("ParallelProjection", ival))
    {
    obj->SetParallelProjection(ival);
    }

  if (elem->GetVectorAttribute("Position", 3, dbuffer3) == 3)
    {
    obj->SetPosition(dbuffer3);
    }

  if (elem->GetVectorAttribute("FocalPoint", 3, dbuffer3) == 3)
    {
    obj->SetFocalPoint(dbuffer3);
    }

  if (elem->GetVectorAttribute("ViewUp", 3, dbuffer3) == 3)
    {
    obj->SetViewUp(dbuffer3);
    }

  if (elem->GetVectorAttribute("ClippingRange", 3, dbuffer3) == 3)
    {
    obj->SetClippingRange(dbuffer3);
    }

  if (elem->GetScalarAttribute("ViewAngle", dval))
    {
    obj->SetViewAngle(dval);
    }

  if (elem->GetScalarAttribute("ParallelScale", dval))
    {
    obj->SetParallelScale(dval);
    }

  return 1;
}


