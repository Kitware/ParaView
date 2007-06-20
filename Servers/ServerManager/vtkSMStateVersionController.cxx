/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStateVersionController.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStateVersionController.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"

vtkStandardNewMacro(vtkSMStateVersionController);
vtkCxxRevisionMacro(vtkSMStateVersionController, "1.1.2.3");
//----------------------------------------------------------------------------
vtkSMStateVersionController::vtkSMStateVersionController()
{
}

//----------------------------------------------------------------------------
vtkSMStateVersionController::~vtkSMStateVersionController()
{
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::Process(vtkPVXMLElement* root)
{
  if (!root || strcmp(root->GetName(), "ServerManagerState") != 0)
    {
    vtkErrorMacro("Invalid root element. Expected \"ServerManagerState\"");
    return false;
    }

  int version[3] = {0, 0, 0};
  bool status = true;
  this->ReadVersion(root, version);

  if (this->GetMajor(version) < 3)
    {
    vtkWarningMacro("State file version is less than 3.0.0, "
      "these states may not work correctly.");
    }

  if (this->GetMajor(version) < 3  || 
    (this->GetMajor(version)==3 && this->GetMinor(version)==0))
    {
    status = status && this->Process_3_0_To_3_1(root) ;
    version[1] = 1;
    }

  return true;
}

//----------------------------------------------------------------------------
bool ConvertViewModulesToViews(
  vtkPVXMLElement* root, void* callData)
{
  vtkSMStateVersionController* self = reinterpret_cast<
    vtkSMStateVersionController*>(callData);
  return self->ConvertViewModulesToViews(root);
}

//----------------------------------------------------------------------------
// Called for every data-object display. We will update scalar color 
// properties since those changed.
bool ConvertScalarColoringForRepresentations(vtkPVXMLElement* root,
  void* vtkNotUsed(callData))
{
  // ScalarMode --> ColorAttributeType
  //  vals: 0/1/2/3 ---> 0 (point-data)
  //      : 4       ---> 1 (cell-data)
  // ColorArray --> ColorArrayName
  //  val remains same.
  unsigned int max = root->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < max; cc++)
    {
    vtkPVXMLElement* child = root->GetNestedElement(cc);
    if (child->GetName() && strcmp(child->GetName(), "Property")==0)
      {
      const char* pname = child->GetAttribute("name");
      if (pname && strcmp(pname, "ColorArray")==0)
        {
        child->SetAttribute("name", "ColorArrayName");
        }
      else if (pname && strcmp(pname, "ScalarMode")==0)
        {
        child->SetAttribute("name", "ColorAttributeType");
        vtkPVXMLElement* valueElement = 
          child->FindNestedElementByName("Element");
        if (valueElement)
          {
          int oldValue = 0;
          valueElement->GetScalarAttribute("value", &oldValue);
          int newValue = (oldValue<=3)? 0 : 1;
          ostrstream valueStr;
          valueStr << newValue << ends;
          valueElement->SetAttribute("value", valueStr.str());
          delete[] valueStr.str();
          }
        }
      }
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::Process_3_0_To_3_1(vtkPVXMLElement* root)
{
    {
    // Remove all MultiViewRenderModule elements.
    const char* attrs[] = {
      "group","rendermodules",
      "type", "MultiViewRenderModule", 0};
    this->SelectAndRemove( root, "Proxy",attrs);
    }

    {
    // Replace all requests for proxies in the group "rendermodules" by
    // "newviews".
    const char* attrs[] = {
      "group", "rendermodules", 0};
    const char* newAttrs[] = {
      "group", "newviews",
      "type", "RenderView", 0};
    this->Select( root, "Proxy", attrs,
      &::ConvertViewModulesToViews,
      this);
    this->SelectAndSetAttributes(root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert different kinds of old-views to new-views.
    const char* attrs[] = {
      "group", "plotmodules",
      "type", "BarChartViewModule", 0};
    const char* newAttrs[] = {
      "group", "newviews",
      "type", "BarChartView", 0};
    this->Select( root, "Proxy", attrs,
      &::ConvertViewModulesToViews,
      this);
    this->SelectAndSetAttributes(root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert different kinds of old-views to new-views.
    const char* attrs[] = {
      "group", "plotmodules",
      "type", "XYPlotViewModule", 0};
    const char* newAttrs[] = {
      "group", "newviews",
      "type", "XYPlotView", 0};
    this->Select( root, "Proxy", attrs,
      &::ConvertViewModulesToViews,
      this);
    this->SelectAndSetAttributes(root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert different kinds of old-views to new-views.
    const char* attrs[] = {
      "group", "views",
      "type", "ElementInspectorView", 0};
    const char* newAttrs[] = {
      "group", "newviews",
      "type", "ElementInspectorView", 0};
    this->Select( root, "Proxy", attrs,
      &::ConvertViewModulesToViews,
      this);
    this->SelectAndSetAttributes(root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert all geometry displays to representations.
    const char* lodAttrs[] = {
      "type", "LODDisplay", 0};
    const char* compositeAttrs[] = {
      "type", "CompositeDisplay", 0};
    const char* multiAttrs[] = {
      "type", "MultiDisplay", 0};

    const char* newAttrs[] = {
      "group", "representations",
      "type", "GeometryRepresentation", 0};

    this->Select(root, "Proxy", lodAttrs,
      &ConvertScalarColoringForRepresentations, this);
    this->SelectAndSetAttributes(
      root, "Proxy", lodAttrs, newAttrs);

    this->Select(root, "Proxy", compositeAttrs,
      &ConvertScalarColoringForRepresentations, this);
    this->SelectAndSetAttributes(
      root, "Proxy", compositeAttrs, newAttrs);

    this->Select(root, "Proxy", multiAttrs,
      &ConvertScalarColoringForRepresentations, this);
    this->SelectAndSetAttributes(
      root, "Proxy", multiAttrs, newAttrs);
    }

    {
    // Convert all text widget displays to representations.
    const char* attrs[] = {
      "type", "TextWidgetDisplay", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "TextWidgetRepresentation", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert all GenericViewDisplay displays to representations.
    const char* attrs[] = {
      "type", "GenericViewDisplay", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "ClientDeliveryRepresentation", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert all displays to representations.
    const char* attrs[] = {
      "type", "BarChartDisplay", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "BarChartRepresentation", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert all displays to representations.
    const char* attrs[] = {
      "type", "XYPlotDisplay2", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "XYPlotRepresentation", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert all displays to representations.
    const char* attrs[] = {
      "type", "ScalarBarWidget", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "ScalarBarWidgetRepresentation", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert all displays to representations.
    const char* attrs[] = {
      "type", "ElementInspectorDisplay", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "ElementInspectorRepresentation", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert Axes to AxesRepresentation.
    const char* attrs[] = {
      "group", "axes", 
      "type", "Axes", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "AxesRepresentation", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Delete all obsolete display types.
    const char* attrsCAD[] = {
      "type", "CubeAxesDisplay",0};
    const char* attrsPLD[] = {
      "type", "PointLabelDisplay",0};

    this->SelectAndRemove(root, "Proxy", attrsCAD);
    this->SelectAndRemove(root, "Proxy", attrsPLD);
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::ConvertViewModulesToViews(
  vtkPVXMLElement* parent)
{
  const char* attrs[] = {
    "name", "Displays", 0 };
  const char* newAttrs[] = {
    "name", "Representations", 0};
  // Replace the "Displays" property with "Representations".
  this->SelectAndSetAttributes(
    parent,
    "Property", attrs, newAttrs);

  // Convert property RenderWindowSize to ViewSize.
  const char* propAttrs[] = {
    "name", "RenderWindowSize", 0};
  const char* propNewAttrs[] = {
    "name", "ViewSize", 0};
  this->SelectAndSetAttributes(
    parent,
    "Property", propAttrs, propNewAttrs);

  return true;
}

//----------------------------------------------------------------------------
void vtkSMStateVersionController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


