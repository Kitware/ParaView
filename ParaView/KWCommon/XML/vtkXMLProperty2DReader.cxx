/*=========================================================================

  Module:    vtkXMLProperty2DReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLProperty2DReader.h"

#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLProperty2DReader);
vtkCxxRevisionMacro(vtkXMLProperty2DReader, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLProperty2DReader::GetRootElementName()
{
  return "Property2D";
}

//----------------------------------------------------------------------------
int vtkXMLProperty2DReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkProperty2D *obj = vtkProperty2D::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The Property2D is not set!");
    return 0;
    }

  // Get attributes

  float fbuffer3[3], fval;
  int ival;

  if (elem->GetVectorAttribute("Color", 3, fbuffer3) == 3)
    {
    obj->SetColor(fbuffer3);
    }

  if (elem->GetScalarAttribute("Opacity", fval))
    {
    obj->SetOpacity(fval);
    }

  if (elem->GetScalarAttribute("PointSize", fval))
    {
    obj->SetPointSize(fval);
    }

  if (elem->GetScalarAttribute("LineWidth", fval))
    {
    obj->SetLineWidth(fval);
    }

  if (elem->GetScalarAttribute("LineStipplePattern", ival))
    {
    obj->SetLineStipplePattern(ival);
    }

  if (elem->GetScalarAttribute("LineStippleRepeatFactor", ival))
    {
    obj->SetLineStippleRepeatFactor(ival);
    }

  if (elem->GetScalarAttribute("DisplayLocation", ival))
    {
    obj->SetDisplayLocation(ival);
    }

  return 1;
}


