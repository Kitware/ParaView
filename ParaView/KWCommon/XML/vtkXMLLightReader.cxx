/*=========================================================================

  Module:    vtkXMLLightReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLLightReader.h"

#include "vtkLight.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLLightReader);
vtkCxxRevisionMacro(vtkXMLLightReader, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLLightReader::GetRootElementName()
{
  return "Light";
}

//----------------------------------------------------------------------------
int vtkXMLLightReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkLight *obj = vtkLight::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The Light is not set!");
    return 0;
    }

  // Get attributes

  float fbuffer3[3], fval;
  int ival;

  if (elem->GetVectorAttribute("Color", 3, fbuffer3) == 3)
    {
    obj->SetColor(fbuffer3);
    }

  if (elem->GetVectorAttribute("Position", 3, fbuffer3) == 3)
    {
    obj->SetPosition(fbuffer3);
    }

  if (elem->GetVectorAttribute("FocalPoint", 3, fbuffer3) == 3)
    {
    obj->SetFocalPoint(fbuffer3);
    }

  if (elem->GetScalarAttribute("Intensity", fval))
    {
    obj->SetIntensity(fval);
    }

  if (elem->GetScalarAttribute("Switch", ival))
    {
    obj->SetSwitch(ival);
    }

  if (elem->GetScalarAttribute("Positional", ival))
    {
    obj->SetPositional(ival);
    }

  if (elem->GetScalarAttribute("Exponent", fval))
    {
    obj->SetExponent(fval);
    }

  if (elem->GetScalarAttribute("ConeAngle", fval))
    {
    obj->SetConeAngle(fval);
    }

  if (elem->GetVectorAttribute("AttenuationValues", 3, fbuffer3) == 3)
    {
    obj->SetAttenuationValues(fbuffer3);
    }

  if (elem->GetScalarAttribute("LightType", ival))
    {
    obj->SetLightType(ival);
    }

  return 1;
}


