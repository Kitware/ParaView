/*=========================================================================

  Module:    vtkXMLLightWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLLightWriter.h"

#include "vtkObjectFactory.h"
#include "vtkLight.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLLightWriter);
vtkCxxRevisionMacro(vtkXMLLightWriter, "1.3");

//----------------------------------------------------------------------------
char* vtkXMLLightWriter::GetRootElementName()
{
  return "Light";
}

//----------------------------------------------------------------------------
int vtkXMLLightWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkLight *obj = vtkLight::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The Light is not set!");
    return 0;
    }

  elem->SetVectorAttribute("Color", 3, obj->GetColor());

  elem->SetVectorAttribute("Position", 3, obj->GetPosition());

  elem->SetVectorAttribute("FocalPoint", 3, obj->GetFocalPoint());

  elem->SetFloatAttribute("Intensity", obj->GetIntensity());

  elem->SetIntAttribute("Switch", obj->GetSwitch());

  elem->SetIntAttribute("Positional", obj->GetPositional());

  elem->SetFloatAttribute("Exponent", obj->GetExponent());

  elem->SetFloatAttribute("ConeAngle", obj->GetConeAngle());

  elem->SetVectorAttribute(
    "AttenuationValues", 3, obj->GetAttenuationValues());

  elem->SetIntAttribute("LightType", obj->GetLightType());

  return 1;
}


