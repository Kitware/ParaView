/*=========================================================================

  Module:    vtkXMLPropertyWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPropertyWriter.h"

#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLPropertyWriter);
vtkCxxRevisionMacro(vtkXMLPropertyWriter, "1.4");

//----------------------------------------------------------------------------
char* vtkXMLPropertyWriter::GetRootElementName()
{
  return "Property";
}

//----------------------------------------------------------------------------
vtkXMLPropertyWriter::vtkXMLPropertyWriter()
{
  this->OutputShadingOnly = 0;
}

//----------------------------------------------------------------------------
int vtkXMLPropertyWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkProperty *obj = vtkProperty::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The Property is not set!");
    return 0;
    }

  elem->SetFloatAttribute("Ambient", obj->GetAmbient());

  elem->SetFloatAttribute("Diffuse", obj->GetDiffuse());

  elem->SetFloatAttribute("Specular", obj->GetSpecular());

  elem->SetFloatAttribute("SpecularPower", obj->GetSpecularPower());

  if (this->OutputShadingOnly)
    {
    return 1;
    }

  elem->SetIntAttribute("Interpolation", obj->GetInterpolation());

  elem->SetIntAttribute("Representation", obj->GetRepresentation());

  elem->SetVectorAttribute("Color", 3, obj->GetColor());

  elem->SetVectorAttribute("AmbientColor", 3, obj->GetAmbientColor());

  elem->SetVectorAttribute("DiffuseColor", 3, obj->GetDiffuseColor());

  elem->SetVectorAttribute("SpecularColor", 3, obj->GetSpecularColor());

  elem->SetFloatAttribute("Opacity", obj->GetOpacity());

  elem->SetIntAttribute("EdgeVisibility", obj->GetEdgeVisibility());

  elem->SetVectorAttribute("EdgeColor", 3, obj->GetEdgeColor());

  elem->SetFloatAttribute("LineWidth", obj->GetLineWidth());

  elem->SetIntAttribute("LineStipplePattern", obj->GetLineStipplePattern());

  elem->SetIntAttribute("LineStippleRepeatFactor", 
                        obj->GetLineStippleRepeatFactor());
  
  elem->SetFloatAttribute("PointSize", obj->GetPointSize());
  
  elem->SetIntAttribute("BackfaceCulling", obj->GetBackfaceCulling());
  
  elem->SetIntAttribute("FrontfaceCulling", obj->GetFrontfaceCulling());

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPropertyWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OutputShadingOnly: "
     << (this->OutputShadingOnly ? "On" : "Off") << endl;
}
