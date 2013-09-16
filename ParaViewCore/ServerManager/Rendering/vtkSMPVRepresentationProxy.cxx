/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPVRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPVRepresentationProxy.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVTemporalDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTransferFunctionProxy.h"

#include <set>
#include <string>

class vtkSMPVRepresentationProxy::vtkStringSet :
  public std::set<std::string> {};

vtkStandardNewMacro(vtkSMPVRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::vtkSMPVRepresentationProxy()
{
  this->SetSIClassName("vtkSIPVRepresentationProxy");
  this->RepresentationSubProxies = new vtkStringSet();
  this->InReadXMLAttributes = false;
}

//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::~vtkSMPVRepresentationProxy()
{
  delete this->RepresentationSubProxies;
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects();
  if (!this->ObjectsCreated)
    {
    return;
    }

  // Ensure that we update the RepresentationTypesInfo property and the domain
  // for "Representations" property before CreateVTKObjects() is finished. This
  // ensure that all representations have valid Representations domain.
  this->UpdatePropertyInformation();

  // Whenever the "Representation" property is modified, we ensure that the
  // this->InvalidateDataInformation() is called.
  this->AddObserver(vtkCommand::UpdatePropertyEvent,
    this, &vtkSMPVRepresentationProxy::OnPropertyUpdated);
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::OnPropertyUpdated(vtkObject*,
  unsigned long, void* calldata)
{
  const char* pname = reinterpret_cast<const char*>(calldata);
  if (pname && strcmp(pname, "Representation") == 0)
    {
    this->InvalidateDataInformation();
    }
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetPropertyModifiedFlag(const char* name, int flag)
{
  if (!this->InReadXMLAttributes && name && strcmp(name, "Input") == 0)
    {
    // Whenever the input for the representation is set, we need to setup the
    // the input for the internal selection representation that shows the
    // extracted-selection. This is done at the proxy level so that whenever the
    // selection is changed in the application, the SelectionRepresentation is
    // 'MarkedModified' correctly, so that it updates itself cleanly.
    vtkSMProxy* selectionRepr = this->GetSubProxy("SelectionRepresentation");
    vtkSMPropertyHelper helper(this, name);
    for (unsigned int cc=0; cc < helper.GetNumberOfElements(); cc++)
      {
      vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(
        helper.GetAsProxy(cc));
      if (input && selectionRepr)
        {
        input->CreateSelectionProxies();
        vtkSMSourceProxy* esProxy = input->GetSelectionOutput(
          helper.GetOutputPort(cc));
        if (!esProxy)
          {
          vtkErrorMacro("Input proxy does not support selection extraction.");
          }
        else
          {
          vtkSMPropertyHelper(selectionRepr, "Input").Set(esProxy);
          selectionRepr->UpdateVTKObjects();
          }
        }
      }
    }

  this->Superclass::SetPropertyModifiedFlag(name, flag);
}

//----------------------------------------------------------------------------
int vtkSMPVRepresentationProxy::ReadXMLAttributes(
  vtkSMSessionProxyManager* pm, vtkPVXMLElement* element)
{
  this->InReadXMLAttributes = true;
  for (unsigned int cc=0; cc < element->GetNumberOfNestedElements(); ++cc)
    {
    vtkPVXMLElement* child = element->GetNestedElement(cc);
    if ( child->GetName() &&
         strcmp(child->GetName(), "RepresentationType") == 0 &&
         child->GetAttribute("subproxy") != NULL )
      {
      this->RepresentationSubProxies->insert(child->GetAttribute("subproxy"));
      }
    }

  int retVal = this->Superclass::ReadXMLAttributes(pm, element);
  this->InReadXMLAttributes = false;

  // Setup property links for sub-proxies. This ensures that whenever the
  // this->GetProperty("Input") changes (either checked or un-checked values),
  // all the sub-proxy's "Input" is also changed to the same value. This ensures
  // that the domains are updated correctly.
  vtkSMProperty* inputProperty = this->GetProperty("Input");
  if (inputProperty)
    {
    for (vtkStringSet::iterator iter = this->RepresentationSubProxies->begin();
      iter != this->RepresentationSubProxies->end(); ++iter)
      {
      vtkSMProxy* subProxy = this->GetSubProxy((*iter).c_str());
      vtkSMProperty* subProperty = subProxy? subProxy->GetProperty("Input") : NULL;
      if (subProperty)
        {
        this->LinkProperty(inputProperty, subProperty);
        }
      }
    }
  return retVal;
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::GetUsingScalarColoring()
{
  if (this->GetProperty("ColorArrayName"))
    {
    vtkSMPropertyHelper helper(this->GetProperty("ColorArrayName"));
    return (helper.GetNumberOfElements() == 1 &&
            helper.GetAsString(0) != NULL &&
            strcmp(helper.GetAsString(0), "") != 0);
    }
  else
    {
    vtkWarningMacro("Missing 'ColorArrayName' property.");
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(
  bool extend)
{
  if (!this->GetUsingScalarColoring())
    {
    // we are not using scalar coloring, nothing to do.
    return false;
    }

  if (!this->GetProperty("ColorAttributeType"))
    {
    vtkWarningMacro("Missing 'ColorAttributeType' property.");
    return false;
    }


  return this->RescaleTransferFunctionToDataRange(
    vtkSMPropertyHelper(this->GetProperty("ColorArrayName")).GetAsString(0),
    vtkSMPropertyHelper(this->GetProperty("ColorAttributeType")).GetAsInt(0),
    extend);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(
  const char* arrayname, int attribute_type, bool extend)
{
  vtkSMPropertyHelper inputHelper(this->GetProperty("Input"));
  vtkSMSourceProxy* inputProxy =
    vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  int port = inputHelper.GetOutputPort();
  if (!inputProxy)
    {
    // no input.
    vtkWarningMacro("No input present. Cannot determine data ranges.");
    return false;
    }
  
  vtkPVDataInformation* dataInfo = inputProxy->GetDataInformation(port);
  vtkPVArrayInformation* info = dataInfo->GetArrayInformation(
    arrayname, attribute_type);
  if (!info)
    {
    vtkPVDataInformation* representedDataInfo =
      this->GetRepresentedDataInformation();
    info = representedDataInfo->GetArrayInformation(arrayname, attribute_type);
    }

  return this->RescaleTransferFunctionToDataRange(info, extend);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRangeOverTime()
{
  if (!this->GetUsingScalarColoring())
    {
    // we are not using scalar coloring, nothing to do.
    return false;
    }

  if (!this->GetProperty("ColorAttributeType"))
    {
    vtkWarningMacro("Missing 'ColorAttributeType' property.");
    return false;
    }


  return this->RescaleTransferFunctionToDataRangeOverTime(
    vtkSMPropertyHelper(this->GetProperty("ColorArrayName")).GetAsString(0),
    vtkSMPropertyHelper(this->GetProperty("ColorAttributeType")).GetAsInt(0));
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRangeOverTime(
  const char* arrayname, int attribute_type)
{
  vtkSMPropertyHelper inputHelper(this->GetProperty("Input"));
  vtkSMSourceProxy* inputProxy =
    vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  int port = inputHelper.GetOutputPort();
  if (!inputProxy || !inputProxy->GetOutputPort(port))
    {
    // no input.
    vtkWarningMacro("No input present. Cannot determine data ranges.");
    return false;
    }
 
  vtkPVTemporalDataInformation* dataInfo =
    inputProxy->GetOutputPort(port)->GetTemporalDataInformation();
  vtkPVArrayInformation* info = dataInfo->GetArrayInformation(
    arrayname, attribute_type);
  return info? this->RescaleTransferFunctionToDataRange(info) : false;
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(
  vtkPVArrayInformation* info, bool extend)
{
  if (!info)
    {
    vtkWarningMacro("Could not determine array range.");
    return false;
    }

  vtkSMProperty* lutProperty = this->GetProperty("LookupTable");
  vtkSMProperty* sofProperty = this->GetProperty("ScalarOpacityFunction");
  if (!lutProperty && !sofProperty)
    {
    vtkWarningMacro("No 'LookupTable' and 'ScalarOpacityFunction' found.");
    return false;
    }

  vtkSMProxy* lut = vtkSMPropertyHelper(lutProperty).GetAsProxy();
  vtkSMProxy* sof = vtkSMPropertyHelper(sofProperty).GetAsProxy();

  // We need to determine the component number to use from the lut.
  int component = -1;
  if (lut && vtkSMPropertyHelper(lut, "VectorMode").GetAsInt() != 0)
    {
    component = vtkSMPropertyHelper(lut, "VectorComponent").GetAsInt();
    }

  if (component < info->GetNumberOfComponents())
    {
    double range[2];
    info->GetComponentRange(component, range);
    if (range[1] >= range[0])
      {
      if ( (range[1] - range[0] < 1e-5) )
        {
        range[1] = range[0] + 1e-5;
        }
      // If data range is too small then we tweak it a bit so scalar mapping
      // produces valid/reproducible results.
      if (lut)
        {
        vtkSMTransferFunctionProxy::RescaleTransferFunction(lut, range, extend);
        vtkSMProxy* sof_lut = vtkSMPropertyHelper(
          lut, "ScalarOpacityFunction", true).GetAsProxy();
        if (sof_lut && sof != sof_lut)
          {
          vtkSMTransferFunctionProxy::RescaleTransferFunction(
            sof_lut, range, extend);
          }
        }
      if (sof)
        {
        vtkSMTransferFunctionProxy::RescaleTransferFunction(sof, range, extend);
        }

      return (lut || sof);
      }

    }
  return false;
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
