/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLLightReader.cxx
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
#include "vtkXMLLightReader.h"

#include "vtkLight.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLLightReader);
vtkCxxRevisionMacro(vtkXMLLightReader, "1.1");

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
