/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPropertyWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkXMLPropertyWriter.h"

#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLPropertyWriter);
vtkCxxRevisionMacro(vtkXMLPropertyWriter, "1.1");

//----------------------------------------------------------------------------
char* vtkXMLPropertyWriter::GetRootElementName()
{
  return "Property";
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

  elem->SetIntAttribute("Interpolation", obj->GetInterpolation());

  elem->SetIntAttribute("Representation", obj->GetRepresentation());

  elem->SetVectorAttribute("Color", 3, obj->GetColor());

  elem->SetFloatAttribute("Ambient", obj->GetAmbient());

  elem->SetFloatAttribute("Diffuse", obj->GetDiffuse());

  elem->SetFloatAttribute("Specular", obj->GetSpecular());

  elem->SetFloatAttribute("SpecularPower", obj->GetSpecularPower());

  elem->SetFloatAttribute("Opacity", obj->GetOpacity());

  elem->SetVectorAttribute("AmbientColor", 3, obj->GetAmbientColor());

  elem->SetVectorAttribute("DiffuseColor", 3, obj->GetDiffuseColor());

  elem->SetVectorAttribute("SpecularColor", 3, obj->GetSpecularColor());

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
