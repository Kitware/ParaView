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
#include "vtkDataObject.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVTemporalDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMTransferFunctionProxy.h"

#include <set>
#include <string>
#include <vtksys/ios/sstream>

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
    return (helper.GetNumberOfElements() == 5 &&
            helper.GetAsString(4) != NULL &&
            strcmp(helper.GetAsString(4), "") != 0);
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

  return this->RescaleTransferFunctionToDataRange(
    this->GetArrayInformationForColorArray(), extend);
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

  vtkSMPropertyHelper helper(this->GetProperty("ColorArrayName"));

  return this->RescaleTransferFunctionToDataRangeOverTime(
    helper.GetAsString(4), helper.GetAsInt(3));
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
      if ( (range[1] - range[0]) < 1e-16 )
        {
        range[1] = range[0] + 1e-16;
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
bool vtkSMPVRepresentationProxy::SetScalarColoring(const char* arrayname, int attribute_type)
{
  if (!this->GetUsingScalarColoring() && (arrayname==NULL ||arrayname[0]==0))
    {
    // scalar coloring already off. Nothing to do.
    return true;
    }

  vtkSMProperty* colorArray = this->GetProperty("ColorArrayName");
  if (!colorArray)
    {
    vtkWarningMacro("No 'ColorArrayName' property found.");
    return false;
    }

  vtkSMPropertyHelper colorArrayHelper(colorArray);
  colorArrayHelper.SetInputArrayToProcess(attribute_type, arrayname);

  if (arrayname == NULL || arrayname[0] == '\0')
    {
    vtkSMPropertyHelper(this, "LookupTable", true).RemoveAllValues();
    vtkSMPropertyHelper(this, "ScalarOpacityFunction", true).RemoveAllValues();
    this->UpdateVTKObjects();
    return true;
    }

  // Now, setup transfer functions.
  vtkNew<vtkSMTransferFunctionManager> mgr;
  if (vtkSMProperty* lutProperty = this->GetProperty("LookupTable"))
    {
    vtkSMProxy* lutProxy =
      mgr->GetColorTransferFunction(arrayname, this->GetSessionProxyManager());
    vtkSMPropertyHelper(lutProperty).Set(lutProxy);
    }

  if (vtkSMProperty* sofProperty = this->GetProperty("ScalarOpacityFunction"))
    {
    vtkSMProxy* sofProxy =
      mgr->GetOpacityTransferFunction(arrayname, this->GetSessionProxyManager());
    vtkSMPropertyHelper(sofProperty).Set(sofProxy);
    }

  this->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::SetScalarBarVisibility(vtkSMProxy* view, bool visibile)
{
  if (!view)
    {
    return false;
    }

  vtkSMProperty* lutProperty = this->GetProperty("LookupTable");
  if (!lutProperty)
    {
    vtkWarningMacro("Missing 'LookupTable' property.");
    return false;
    }

  vtkSMPropertyHelper lutPropertyHelper(lutProperty);
  if (lutPropertyHelper.GetNumberOfElements() == 0 ||
    lutPropertyHelper.GetAsProxy(0) == NULL)
    {
    vtkWarningMacro("Failed to determine the LookupTable being used.");
    return false;
    }

  vtkSMProxy* lutProxy = lutPropertyHelper.GetAsProxy(0);

  // if hiding the Scalar Bar, just look if there's a LUT and then hide the
  // corresponding scalar bar. We won't worry too much about whether scalar
  // coloring is currently enabled for this.
  if (!visibile)
    {
    if (vtkSMProxy* sbProxy = vtkSMTransferFunctionProxy::FindScalarBarRepresentation(
        lutPropertyHelper.GetAsProxy(), view))
      {
      vtkSMPropertyHelper(sbProxy, "Visibility").Set(0);
      vtkSMPropertyHelper(sbProxy, "Enabled").Set(0);
      sbProxy->UpdateVTKObjects();
      }
    return true;
    }

  if (!this->GetUsingScalarColoring())
    {
    return false;
    }

  vtkNew<vtkSMTransferFunctionManager> mgr;
  vtkSMProxy* sbProxy = mgr->GetScalarBarRepresentation(lutProxy, view);
  if (!sbProxy)
    {
    vtkWarningMacro("Failed to locate/create ScalarBar representation.");
    return false;
    }

  vtkSMPropertyHelper(sbProxy, "Enabled").Set(1);
  vtkSMPropertyHelper(sbProxy, "Visibility").Set(1);

  vtkSMProperty* titleProp = sbProxy->GetProperty("Title");
  vtkSMProperty* compProp = sbProxy->GetProperty("ComponentTitle");
  if (titleProp && compProp &&
      titleProp->IsValueDefault() &&
      compProp->IsValueDefault())
    {
    vtkSMPropertyHelper colorArrayHelper(this, "ColorArrayName");
    vtkSMPropertyHelper(titleProp).Set(
      colorArrayHelper.GetInputArrayNameToProcess());
    // now, determine a name for it if possible.
    vtkPVArrayInformation* arrayInfo = this->GetArrayInformationForColorArray();
    vtkSMScalarBarWidgetRepresentationProxy::UpdateComponentTitle(sbProxy, arrayInfo);
    }
  vtkSMScalarBarWidgetRepresentationProxy::PlaceInView(sbProxy, view);
  sbProxy->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::HideScalarBarIfNotNeeded(vtkSMProxy* view)
{
  vtkSMProperty* lutProperty = this->GetProperty("LookupTable");
  if (!lutProperty)
    {
    return false;
    }

  vtkSMPropertyHelper lutPropertyHelper(lutProperty);
  if (lutPropertyHelper.GetNumberOfElements() == 0 ||
    lutPropertyHelper.GetAsProxy(0) == NULL)
    {
    return false;
    }

  vtkSMProxy* lutProxy = lutPropertyHelper.GetAsProxy(0);
  vtkNew<vtkSMTransferFunctionManager> tmgr;
  return tmgr->HideScalarBarIfNotNeeded(lutProxy, view);
}

//----------------------------------------------------------------------------
vtkPVArrayInformation* vtkSMPVRepresentationProxy::GetArrayInformationForColorArray()
{
  if (!this->GetUsingScalarColoring())
    {
    return NULL;
    }

  // now, determine a name for it if possible.
  vtkSMPropertyHelper colorArrayHelper(this, "ColorArrayName");
  vtkPVArrayInformation* arrayInfo =
    this->GetRepresentedDataInformation()->GetArrayInformation(
      colorArrayHelper.GetInputArrayNameToProcess(),
      colorArrayHelper.GetInputArrayAssociation());
  if (arrayInfo)
    {
    return arrayInfo;
    }

  vtkSMPropertyHelper inputHelper(this, "Input");
  vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  unsigned int port = inputHelper.GetOutputPort();
  if (input)
    {
    return input->GetDataInformation(port)->GetArrayInformation(
      colorArrayHelper.GetInputArrayNameToProcess(),
      colorArrayHelper.GetInputArrayAssociation());
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkPVProminentValuesInformation*
vtkSMPVRepresentationProxy::GetProminentValuesInformationForColorArray(
  double uncertaintyAllowed, double fraction)
{
  if (!this->GetUsingScalarColoring())
    {
    return NULL;
    }

  vtkPVArrayInformation* arrayInfo = this->GetArrayInformationForColorArray();
  if (!arrayInfo)
    {
    return NULL;
    }

  vtkSMPropertyHelper colorArrayHelper(this, "ColorArrayName");
  return this->GetProminentValuesInformation(
    arrayInfo->GetName(), colorArrayHelper.GetInputArrayAssociation(),
    arrayInfo->GetNumberOfComponents(),
    uncertaintyAllowed, fraction);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::SetRepresentationType(const char* type)
{
  const int CALL_SUPERCLASS = 0;
  try
    {
    if (type == 0)
      {
      throw CALL_SUPERCLASS;
      }

    if (strcmp(type, "Volume") != 0 && strcmp(type, "Slice") != 0)
      {
      throw CALL_SUPERCLASS;
      }

    // we are changing to volume or slice representation.
    if (this->GetUsingScalarColoring())
      {
      throw CALL_SUPERCLASS;
      }

    // pick a color array and then accept or fail as applicable.
    vtkSMProperty* colorArrayName = this->GetProperty("ColorArrayName");
    if (colorArrayName)
      {
      vtkSMArrayListDomain* ald = vtkSMArrayListDomain::SafeDownCast(
        colorArrayName->FindDomain("vtkSMArrayListDomain"));

      if (ald && ald->GetNumberOfStrings() > 0)
        {
        unsigned int index=0;
        // if possible, pick a "point" array since that works better with some
        // crappy volume renderers. We need to fixed all volume mapper to not
        // segfault when cell data is picked.
        for (unsigned int cc=0, max=ald->GetNumberOfStrings(); cc < max; cc++)
          {
          if (ald->GetFieldAssociation(cc) == vtkDataObject::POINT)
            {
            index = cc;
            break;
            }
          }
        if (this->SetScalarColoring(ald->GetString(index), ald->GetFieldAssociation(index)))
          {
          throw CALL_SUPERCLASS;
          }
        }
      }
    }
  catch (int val)
    {
    if (val == CALL_SUPERCLASS)
      {
      return this->Superclass::SetRepresentationType(type);
      }
    }
  // It's not sure if the we should error out or still do the change. Opting for
  // going further with the change in representation type for now.
  return this->Superclass::SetRepresentationType(type);
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
