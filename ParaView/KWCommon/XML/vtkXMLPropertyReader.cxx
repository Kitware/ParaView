/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkXMLPropertyReader.h"

#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLPropertyReader);
vtkCxxRevisionMacro(vtkXMLPropertyReader, "1.2");

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

  float fbuffer3[3], fval;
  int ival;

  if (elem->GetScalarAttribute("Interpolation", ival))
    {
    obj->SetInterpolation(ival);
    }

  if (elem->GetScalarAttribute("Representation", ival))
    {
    obj->SetRepresentation(ival);
    }

  if (elem->GetVectorAttribute("Color", 3, fbuffer3) == 3)
    {
    obj->SetColor(fbuffer3);
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

  if (elem->GetVectorAttribute("AmbientColor", 3, fbuffer3) == 3)
    {
    obj->SetAmbientColor(fbuffer3);
    }

  if (elem->GetVectorAttribute("DiffuseColor", 3, fbuffer3) == 3)
    {
    obj->SetDiffuseColor(fbuffer3);
    }

  if (elem->GetVectorAttribute("SpecularColor", 3, fbuffer3) == 3)
    {
    obj->SetSpecularColor(fbuffer3);
    }

  if (elem->GetScalarAttribute("EdgeVisibility", ival))
    {
    obj->SetEdgeVisibility(ival);
    }

  if (elem->GetVectorAttribute("EdgeColor", 3, fbuffer3) == 3)
    {
    obj->SetEdgeColor(fbuffer3);
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


