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
#include "vtkXMLVolumePropertyWriter.h"

#include "vtkColorTransferFunction.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkVolumeProperty.h"
#include "vtkXMLColorTransferFunctionWriter.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLPiecewiseFunctionWriter.h"

vtkStandardNewMacro(vtkXMLVolumePropertyWriter);
vtkCxxRevisionMacro(vtkXMLVolumePropertyWriter, "1.9");

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
vtkXMLVolumePropertyWriter::vtkXMLVolumePropertyWriter()
{
  this->OutputShadingOnly = 0;
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

  if (this->OutputShadingOnly)
    {
    return 1;
    }

  elem->SetIntAttribute(
    "InterpolationType", obj->GetInterpolationType());

  elem->SetIntAttribute(
    "IndependentComponents", obj->GetIndependentComponents());

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
    vtkXMLDataElement *comp_elem = this->NewDataElement();
    elem->AddNestedElement(comp_elem);
    comp_elem->Delete();
    comp_elem->SetName(this->GetComponentElementName());

    comp_elem->SetIntAttribute("Index", c_idx);

    comp_elem->SetIntAttribute("Shade", obj->GetShade(c_idx));
    comp_elem->SetFloatAttribute("Ambient", obj->GetAmbient(c_idx));
    comp_elem->SetFloatAttribute("Diffuse", obj->GetDiffuse(c_idx));
    comp_elem->SetFloatAttribute("Specular", obj->GetSpecular(c_idx));
    comp_elem->SetFloatAttribute(
      "SpecularPower", obj->GetSpecularPower(c_idx));

    if (this->OutputShadingOnly)
      {
      continue;
      }

    comp_elem->SetIntAttribute(
      "ColorChannels", obj->GetColorChannels(c_idx));

    comp_elem->SetIntAttribute(
      "DisableGradientOpacity", obj->GetDisableGradientOpacity(c_idx));

    comp_elem->SetFloatAttribute(
      "ComponentWeight", obj->GetComponentWeight(c_idx));
    
    comp_elem->SetFloatAttribute(
      "ScalarOpacityUnitDistance", obj->GetScalarOpacityUnitDistance(c_idx));
    
    // Gray or Color Transfer Function

    if (obj->GetColorChannels() == 1)
      {
      vtkPiecewiseFunction *gtf = obj->GetGrayTransferFunction(c_idx);
      if (gtf)
        {
        xmlpfw->SetObject(gtf);
        xmlpfw->CreateInNestedElement(
          comp_elem, this->GetGrayTransferFunctionElementName());
        }
      }
    else if (obj->GetColorChannels() >= 1)
      {
      vtkColorTransferFunction *rgbtf = obj->GetRGBTransferFunction(c_idx);
      if (rgbtf)
        {
        xmlctfw->SetObject(rgbtf);
        xmlctfw->CreateInNestedElement(
          comp_elem, this->GetRGBTransferFunctionElementName());
        }
      }

    // Scalar Opacity

    vtkPiecewiseFunction *sotf = obj->GetScalarOpacity(c_idx);
    if (sotf)
      {
      xmlpfw->SetObject(sotf);
      xmlpfw->CreateInNestedElement(
        comp_elem, this->GetScalarOpacityElementName());
      }

    // Gradient Opacity

    vtkPiecewiseFunction *gotf = obj->GetStoredGradientOpacity(c_idx);
    if (gotf)
      {
      xmlpfw->SetObject(gotf);
      xmlpfw->CreateInNestedElement(
        comp_elem, this->GetGradientOpacityElementName());
      }
    }

  xmlpfw->Delete();
  xmlctfw->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLVolumePropertyWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OutputShadingOnly: "
     << (this->OutputShadingOnly ? "On" : "Off") << endl;
}
