/*=========================================================================

  Module:    vtkXMLPropertyReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPropertyReader.h"

#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLPropertyReader);
vtkCxxRevisionMacro(vtkXMLPropertyReader, "1.4");

//----------------------------------------------------------------------------
char* vtkXMLPropertyReader::GetRootElementName()
{
  return "Property";
}

//----------------------------------------------------------------------------
int vtkXMLPropertyReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkProperty *obj = vtkProperty::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The Property is not set!");
    return 0;
    }

  // Get attributes

  double dbuffer3[3];
  float fval;
  int ival;

  if (elem->GetScalarAttribute("Interpolation", ival))
    {
    obj->SetInterpolation(ival);
    }

  if (elem->GetScalarAttribute("Representation", ival))
    {
    obj->SetRepresentation(ival);
    }

  if (elem->GetVectorAttribute("Color", 3, dbuffer3) == 3)
    {
    obj->SetColor(dbuffer3);
    }

  if (elem->GetScalarAttribute("Ambient", fval))
    {
    obj->SetAmbient(fval);
    }

  if (elem->GetScalarAttribute("Diffuse", fval))
    {
    obj->SetDiffuse(fval);
    }

  if (elem->GetScalarAttribute("Specular", fval))
    {
    obj->SetSpecular(fval);
    }

  if (elem->GetScalarAttribute("SpecularPower", fval))
    {
    obj->SetSpecularPower(fval);
    }

  if (elem->GetScalarAttribute("Opacity", fval))
    {
    obj->SetOpacity(fval);
    }

  if (elem->GetVectorAttribute("AmbientColor", 3, dbuffer3) == 3)
    {
    obj->SetAmbientColor(dbuffer3);
    }

  if (elem->GetVectorAttribute("DiffuseColor", 3, dbuffer3) == 3)
    {
    obj->SetDiffuseColor(dbuffer3);
    }

  if (elem->GetVectorAttribute("SpecularColor", 3, dbuffer3) == 3)
    {
    obj->SetSpecularColor(dbuffer3);
    }

  if (elem->GetScalarAttribute("EdgeVisibility", ival))
    {
    obj->SetEdgeVisibility(ival);
    }

  if (elem->GetVectorAttribute("EdgeColor", 3, dbuffer3) == 3)
    {
    obj->SetEdgeColor(dbuffer3);
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

  if (elem->GetScalarAttribute("PointSize", fval))
    {
    obj->SetPointSize(fval);
    }

  if (elem->GetScalarAttribute("BackfaceCulling", ival))
    {
    obj->SetBackfaceCulling(ival);
    }

  if (elem->GetScalarAttribute("FrontfaceCulling", ival))
    {
    obj->SetFrontfaceCulling(ival);
    }

  return 1;
}


