/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLVolumePropertyWriter.cxx
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
#include "vtkXMLVolumePropertyWriter.h"

#include "vtkColorTransferFunction.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkVolumeProperty.h"
#include "vtkXMLColorTransferFunctionWriter.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLPiecewiseFunctionWriter.h"

vtkStandardNewMacro(vtkXMLVolumePropertyWriter);
vtkCxxRevisionMacro(vtkXMLVolumePropertyWriter, "1.1");

//----------------------------------------------------------------------------
char* vtkXMLVolumePropertyWriter::GetRootElementName()
{
  return "VolumeProperty";
}

//----------------------------------------------------------------------------
char* vtkXMLVolumePropertyWriter::GetComponentElementName()
{
  return "Component";
}

//----------------------------------------------------------------------------
char* vtkXMLVolumePropertyWriter::GetGrayTransferFunctionElementName()
{
  return "GrayTransferFunction";
}

//----------------------------------------------------------------------------
char* vtkXMLVolumePropertyWriter::GetRGBTransferFunctionElementName()
{
  return "RGBTransferFunction";
}

//----------------------------------------------------------------------------
char* vtkXMLVolumePropertyWriter::GetScalarOpacityElementName()
{
  return "ScalarOpacity";
}

//----------------------------------------------------------------------------
char* vtkXMLVolumePropertyWriter::GetGradientOpacityElementName()
{
  return "GradientOpacity";
}

//----------------------------------------------------------------------------
int vtkXMLVolumePropertyWriter::AddAttributes(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddAttributes(elem))
    {
    return 0;
    }

  vtkVolumeProperty *obj = vtkVolumeProperty::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The VolumeProperty is not set!");
    return 0;
    }

  elem->SetIntAttribute("InterpolationType", obj->GetInterpolationType());

  elem->SetIntAttribute("NumberOfComponents", VTK_MAX_VRCOMP);

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLVolumePropertyWriter::AddNestedElements(vtkXMLDataElement *elem)
{
  if (!this->Superclass::AddNestedElements(elem))
    {
    return 0;
    }

  vtkVolumeProperty *obj = vtkVolumeProperty::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The VolumeProperty is not set!");
    return 0;
    }

  // Iterate over all components and 
  // create a component XML data element for each one

  vtkXMLPiecewiseFunctionWriter *xmlpfw =
    vtkXMLPiecewiseFunctionWriter::New();

  vtkXMLColorTransferFunctionWriter *xmlctfw = 
    vtkXMLColorTransferFunctionWriter::New();

  int c_idx;
  for (c_idx = 0; c_idx < VTK_MAX_VRCOMP; c_idx++)
    {
    vtkXMLDataElement *comp_elem = vtkXMLDataElement::New();
    elem->AddNestedElement(comp_elem);
    comp_elem->Delete();
    comp_elem->SetName(this->GetComponentElementName());
    comp_elem->SetIntAttribute("Index", c_idx);
    comp_elem->SetIntAttribute("ColorChannels", obj->GetColorChannels(c_idx));
    comp_elem->SetIntAttribute("Shade", obj->GetShade(c_idx));
    comp_elem->SetFloatAttribute("Ambient", obj->GetAmbient(c_idx));
    comp_elem->SetFloatAttribute("Diffuse", obj->GetDiffuse(c_idx));
    comp_elem->SetFloatAttribute("Specular", obj->GetSpecular(c_idx));
    comp_elem->SetFloatAttribute("SpecularPower", obj->GetSpecularPower(c_idx));
    
    // Gray or Color Transfer Function

    if (obj->GetColorChannels() == 1)
      {
      vtkPiecewiseFunction *gtf = obj->GetGrayTransferFunction(c_idx);
      if (gtf)
        {
        vtkXMLDataElement *gtf_elem = vtkXMLDataElement::New();
        comp_elem->AddNestedElement(gtf_elem);
        gtf_elem->Delete();
        gtf_elem->SetName(this->GetGrayTransferFunctionElementName());
        vtkXMLDataElement *gtf_nested_elem = vtkXMLDataElement::New();
        gtf_elem->AddNestedElement(gtf_nested_elem);
        gtf_nested_elem->Delete();
        xmlpfw->SetObject(gtf);
        xmlpfw->Create(gtf_nested_elem);
        }
      }
    else if (obj->GetColorChannels() >= 1)
      {
      vtkColorTransferFunction *rgbtf = obj->GetRGBTransferFunction(c_idx);
      if (rgbtf)
        {
        vtkXMLDataElement *rgbtf_elem = vtkXMLDataElement::New();
        comp_elem->AddNestedElement(rgbtf_elem);
        rgbtf_elem->Delete();
        rgbtf_elem->SetName(this->GetRGBTransferFunctionElementName());
        vtkXMLDataElement *rgbtf_nested_elem = vtkXMLDataElement::New();
        rgbtf_elem->AddNestedElement(rgbtf_nested_elem);
        rgbtf_nested_elem->Delete();
        xmlctfw->SetObject(rgbtf);
        xmlctfw->Create(rgbtf_nested_elem);
        }
      }

    // Scalar Opacity

    vtkPiecewiseFunction *sotf = obj->GetScalarOpacity(c_idx);
    if (sotf)
      {
      vtkXMLDataElement *sotf_elem = vtkXMLDataElement::New();
      comp_elem->AddNestedElement(sotf_elem);
      sotf_elem->Delete();
      sotf_elem->SetName(this->GetScalarOpacityElementName());
      vtkXMLDataElement *sotf_nested_elem = vtkXMLDataElement::New();
      sotf_elem->AddNestedElement(sotf_nested_elem);
      sotf_nested_elem->Delete();
      xmlpfw->SetObject(sotf);
      xmlpfw->Create(sotf_nested_elem);
      }

    // Gradient Opacity

    vtkPiecewiseFunction *gotf = obj->GetGradientOpacity(c_idx);
    if (gotf)
      {
      vtkXMLDataElement *gotf_elem = vtkXMLDataElement::New();
      comp_elem->AddNestedElement(gotf_elem);
      gotf_elem->Delete();
      gotf_elem->SetName(this->GetGradientOpacityElementName());
      vtkXMLDataElement *gotf_nested_elem = vtkXMLDataElement::New();
      gotf_elem->AddNestedElement(gotf_nested_elem);
      gotf_nested_elem->Delete();
      xmlpfw->SetObject(gotf);
      xmlpfw->Create(gotf_nested_elem);
      }
    }

  xmlpfw->Delete();
  xmlctfw->Delete();

  return 1;
}
