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
vtkCxxRevisionMacro(vtkSMStateVersionController, "1.1.2.1");
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
    vtkErrorMacro("Cannot load state files from version less than 3.0.0");
    return false;
    }

  if (this->GetMajor(version) == 3 && this->GetMinor(version)==0)
    {
    status = status && this->Process_3_0_To_3_1(root) ;
    version[1] = 1;
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionControllerConvertRenderModulesToViews(
  vtkPVXMLElement* root, void* callData)
{
  vtkSMStateVersionController* self = reinterpret_cast<
    vtkSMStateVersionController*>(callData);
  return self->ConvertRenderModulesToViews(root);
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
      &vtkSMStateVersionControllerConvertRenderModulesToViews,
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
      &vtkSMStateVersionControllerConvertRenderModulesToViews,
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
      &vtkSMStateVersionControllerConvertRenderModulesToViews,
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
      &vtkSMStateVersionControllerConvertRenderModulesToViews,
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
    this->SelectAndSetAttributes(
      root, "Proxy", lodAttrs, newAttrs);

    this->SelectAndSetAttributes(
      root, "Proxy", compositeAttrs, newAttrs);

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
      "type", "ElementInspectorDisplay", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "ElementInspectorRepresentation", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::ConvertRenderModulesToViews(
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
  return true;
}

//----------------------------------------------------------------------------
void vtkSMStateVersionController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


