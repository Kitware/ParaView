/*=========================================================================

  Module:    vtkXMLVolumePropertyWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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
vtkCxxRevisionMacro(vtkXMLVolumePropertyWriter, "1.11");

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
  this->NumberOfComponents = VTK_MAX_VRCOMP;
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
  for (c_idx = 0; c_idx < this->NumberOfComponents; c_idx++)
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
