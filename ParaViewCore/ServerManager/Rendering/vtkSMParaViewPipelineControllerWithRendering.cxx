/*=========================================================================

  Program:   ParaView
  Module:    vtkSMParaViewPipelineControllerWithRendering.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMParaViewPipelineControllerWithRendering.h"

#include "vtkCollection.h"
#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMUtilities.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"

#include <cassert>
#include <string>

namespace
{
//---------------------------------------------------------------------------
const char* vtkFindTypeFromHints(vtkPVXMLElement* hints, const int outputPort, const char* xmlTag,
  const char* xmlAttributeName = NULL, const char* xmlAttributeValue = NULL)
{
  if (!hints)
  {
    return NULL;
  }
  for (unsigned int cc = 0, max = hints->GetNumberOfNestedElements(); cc < max; cc++)
  {
    vtkPVXMLElement* child = hints->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), xmlTag) == 0)
    {
      int port;
      // If port exists, then it must match the port number for this port.
      if (child->GetScalarAttribute("port", &port) && (port != outputPort))
      {
        continue;
      }
      // if xmlAttributeName and xmlAttributeValue are provided, the XML must match the
      // (name,value) pair, if present.
      if (xmlAttributeValue && xmlAttributeName && child->GetAttribute(xmlAttributeName))
      {
        if (strcmp(child->GetAttribute(xmlAttributeName), xmlAttributeValue) != 0)
        {
          continue;
        }
      }
      if (const char* type = child->GetAttribute("type"))
      {
        return type;
      }
    }
  }
  return NULL;
}

//---------------------------------------------------------------------------
bool vtkIsOutputTypeNonStandard(vtkPVXMLElement* hints, const int outputPort)
{
  if (!hints)
  {
    return false;
  }
  for (unsigned int cc = 0, max = hints->GetNumberOfNestedElements(); cc < max; cc++)
  {
    vtkPVXMLElement* child = hints->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), "OutputPort") == 0)
    {
      int port;
      // If port exists, then it must match the port number for this port.
      if (child->GetScalarAttribute("port", &port) && (port != outputPort))
      {
        continue;
      }
      if (const char* type = child->GetAttribute("type"))
      {
        if (strcmp(type, "text") == 0)
        {
          return true;
        }
        else if (strcmp(type, "progress") == 0)
        {
          return true;
        }
      }
    }
  }
  return false;
}

//---------------------------------------------------------------------------
void vtkInheritRepresentationProperties(vtkSMRepresentationProxy* repr, vtkSMSourceProxy* producer,
  unsigned int producerPort, vtkSMViewProxy* view, const unsigned long initTimeStamp)
{
  if (producer->GetProperty("Input") == NULL)
  {
    // if producer is not a filter, nothing to do.
    return;
  }

  vtkSMPropertyHelper inputHelper(producer, "Input", true);
  vtkSMProxy* inputRepr = view->FindRepresentation(
    vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy()), inputHelper.GetOutputPort());
  if (inputRepr == NULL)
  {
    // if producer's input has no representation in the view, nothing to do.
    return;
  }

  // Irrespective of other properties, scalar coloring is inherited if
  // possible.
  if (vtkSMPVRepresentationProxy::GetUsingScalarColoring(inputRepr) &&
    !vtkSMPVRepresentationProxy::GetUsingScalarColoring(repr))
  {
    vtkSMPropertyHelper colorArrayHelper(inputRepr, "ColorArrayName");
    const char* arrayName = colorArrayHelper.GetInputArrayNameToProcess();
    int arrayAssociation = colorArrayHelper.GetInputArrayAssociation();

    if (producer->GetDataInformation(producerPort)
          ->GetArrayInformation(arrayName, arrayAssociation))
    {
      vtkSMPVRepresentationProxy::SetScalarColoring(repr,
        colorArrayHelper.GetInputArrayNameToProcess(), colorArrayHelper.GetInputArrayAssociation());
    }
  }

  if (!vtkSMParaViewPipelineControllerWithRendering::GetInheritRepresentationProperties())
  {
    return;
  }
  // copy properties from inputRepr to repr is they weren't modified.
  vtkSMPropertyIterator* iter = inputRepr->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
  {
    const char* pname = iter->GetKey();
    vtkSMProperty* dest = repr->GetProperty(pname);
    vtkSMProperty* source = iter->GetProperty();
    if (pname && (strcmp(pname, "ColorArrayName") == 0 || strcmp(pname, "LookupTable") == 0 ||
                   strcmp(pname, "ScalarOpacityFunction") == 0))
    {
      // HACK: to fix BUG #15539. We avoid copying coloring properties since
      // they are already inherited if needed. The tricky question is how do
      // we inherit data-dependent properties using a generic API? We need a
      // domain-aware-copy method. This method will copy values from a source
      // property that are valid for the destination property's domain. It of
      // course gets more complicated for this like the
      // LookupTable/ScalarOpacityFunction properties. How are those to be
      // copied over esp. since they depend on how ColorArrayName property was
      // copied.
      continue;
    }
    if (vtkSMProxyProperty::SafeDownCast(source))
    {
      // HACK: we skip proxy properties. Without this change, the properties
      // like GlyphType end up inheriting the value from the upstream
      // representation, which is incorrect.
      continue;
    }
    if (dest && source &&
      // the property wasn't modified since initialization or if it is
      // "Representation" property -- (HACK)
      (dest->GetMTime() < initTimeStamp || strcmp("Representation", pname) == 0) &&
      // the property types match.
      strcmp(dest->GetClassName(), source->GetClassName()) == 0)
    {
      dest->Copy(source);
    }
  }
  iter->Delete();
  repr->UpdateVTKObjects();
}

//---------------------------------------------------------------------------
void vtkPickRepresentationType(vtkSMRepresentationProxy* repr, vtkSMSourceProxy* producer,
  unsigned int outputPort, vtkSMViewProxy* view)
{
  (void)producer;
  (void)outputPort;

  // Check if there's a hint for the producer. If so, use that.
  if (const char* reprtype = vtkFindTypeFromHints(
        producer->GetHints(), outputPort, "RepresentationType", "view", view->GetXMLName()))
  {
    if (repr->SetRepresentationType(reprtype))
    {
      return;
    }
  }
  // currently, this just ensures that the "Representation" type chosen has
  // proper color type etc. setup. At some point, we could deprecate
  // vtkSMRepresentationTypeDomain and let this logic pick the default
  // representation type.
  if (vtkSMProperty* smproperty = repr->GetProperty("Representation"))
  {
    repr->SetRepresentationType(vtkSMPropertyHelper(smproperty).GetAsString());
  }
}
}

bool vtkSMParaViewPipelineControllerWithRendering::HideScalarBarOnHide = true;
bool vtkSMParaViewPipelineControllerWithRendering::InheritRepresentationProperties = false;

vtkObjectFactoryNewMacro(vtkSMParaViewPipelineControllerWithRendering);
//----------------------------------------------------------------------------
vtkSMParaViewPipelineControllerWithRendering::vtkSMParaViewPipelineControllerWithRendering()
{
}

//----------------------------------------------------------------------------
vtkSMParaViewPipelineControllerWithRendering::~vtkSMParaViewPipelineControllerWithRendering()
{
}

//----------------------------------------------------------------------------
void vtkSMParaViewPipelineControllerWithRendering::SetHideScalarBarOnHide(bool val)
{
  vtkSMParaViewPipelineControllerWithRendering::HideScalarBarOnHide = val;
}

//----------------------------------------------------------------------------
void vtkSMParaViewPipelineControllerWithRendering::SetInheritRepresentationProperties(bool val)
{
  vtkSMParaViewPipelineControllerWithRendering::InheritRepresentationProperties = val;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineControllerWithRendering::GetInheritRepresentationProperties()
{
  return vtkSMParaViewPipelineControllerWithRendering::InheritRepresentationProperties;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineControllerWithRendering::PostInitializeProxy(vtkSMProxy* proxy)
{
  // save current time, we can check is a property is modified by the superclass
  // call.
  vtkTimeStamp ts;
  ts.Modified();

  if (!this->Superclass::PostInitializeProxy(proxy))
  {
    return false;
  }
  // BUG #14773: The domains for ColorArrayName and Representation properties
  // come up with a good default separately. In reality, we need the
  // ColorArrayName to depend on Representation and not pick any value when
  // using Outline representation. However, since ColorArrayName is on a property
  // on a subproxy while Representation is a property on the outer proxy, we
  // cannot add dependency between the two. So we explicitly manage that here.
  // Note that if the user set the ColorArrayName manually, we should not be
  // changing it here, hence the check of initTime.
  vtkSMProperty* colorArrayName = proxy->GetProperty("ColorArrayName");
  vtkSMProperty* representation = proxy->GetProperty("Representation");
  if (colorArrayName && representation && (colorArrayName->GetMTime() > ts))
  {
    vtkSMPropertyHelper helperRep(representation);
    if (helperRep.GetAsString(0) && strcmp(helperRep.GetAsString(0), "Outline") == 0)
    {
      vtkSMPropertyHelper helper(colorArrayName);
      helper.SetInputArrayToProcess(helper.GetInputArrayAssociation(), "");
      proxy->UpdateVTKObjects();
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineControllerWithRendering::RegisterRepresentationProxy(vtkSMProxy* proxy)
{
  if (!this->Superclass::RegisterRepresentationProxy(proxy))
  {
    return false;
  }

  if (vtkSMPVRepresentationProxy::GetUsingScalarColoring(proxy))
  {
    // If representation has been initialized to use scalar coloring and no
    // transfer functions are setup, we setup the transfer functions.
    vtkSMPropertyHelper helper(proxy, "ColorArrayName");
    const char* arrayName = helper.GetInputArrayNameToProcess();
    if (arrayName != NULL && arrayName[0] != '\0')
    {
      vtkNew<vtkSMTransferFunctionManager> mgr;
      if (vtkSMProperty* sofProperty = proxy->GetProperty("ScalarOpacityFunction"))
      {
        vtkSMProxy* sofProxy =
          mgr->GetOpacityTransferFunction(arrayName, proxy->GetSessionProxyManager());
        vtkSMPropertyHelper(sofProperty).Set(sofProxy);
      }
      if (vtkSMProperty* lutProperty = proxy->GetProperty("LookupTable"))
      {
        vtkSMProxy* lutProxy =
          mgr->GetColorTransferFunction(arrayName, proxy->GetSessionProxyManager());
        vtkSMPropertyHelper(lutProperty).Set(lutProxy);
        vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(proxy, true);
      }
    }
  }
  return true;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMParaViewPipelineControllerWithRendering::Show(
  vtkSMSourceProxy* producer, int outputPort, vtkSMViewProxy* view)
{
  if (producer == NULL || static_cast<int>(producer->GetNumberOfOutputPorts()) <= outputPort)
  {
    vtkErrorMacro("Invalid producer (" << producer << ") or outputPort (" << outputPort << ")");
    return NULL;
  }

  if (view == NULL)
  {
    view = this->ShowInPreferredView(producer, outputPort, NULL);
    return (view ? view->FindRepresentation(producer, outputPort) : NULL);
  }

  // find is there's already a representation in this view.
  if (vtkSMProxy* repr = view->FindRepresentation(producer, outputPort))
  {
    SM_SCOPED_TRACE(Show)
      .arg("producer", producer)
      .arg("port", outputPort)
      .arg("view", view)
      .arg("display", repr);

    vtkSMPropertyHelper(repr, "Visibility").Set(1);
    repr->UpdateVTKObjects();

    vtkSMViewProxy::HideOtherRepresentationsIfNeeded(view, repr);
    return repr;
  }

  // update pipeline to create correct representation type.
  this->UpdatePipelineBeforeDisplay(producer, outputPort, view);

  // Since no repr exists, create a new one if possible.
  if (vtkSMRepresentationProxy* repr = view->CreateDefaultRepresentation(producer, outputPort))
  {
    SM_SCOPED_TRACE(Show)
      .arg("producer", producer)
      .arg("port", outputPort)
      .arg("view", view)
      .arg("display", repr);

    this->PreInitializeProxy(repr);

    vtkTimeStamp ts;
    ts.Modified();

    vtkSMPropertyHelper(repr, "Visibility").Set(1);
    vtkSMPropertyHelper(repr, "Input").Set(producer, outputPort);
    this->PostInitializeProxy(repr);

    // check some setting and then inherit properties.
    vtkInheritRepresentationProperties(repr, producer, outputPort, view, ts);

    // pick good representation type.
    vtkPickRepresentationType(repr, producer, outputPort, view);

    this->RegisterRepresentationProxy(repr);
    repr->UpdateVTKObjects();

    vtkSMPropertyHelper(view, "Representations").Add(repr);
    view->UpdateVTKObjects();
    repr->FastDelete();

    vtkSMViewProxy::HideOtherRepresentationsIfNeeded(view, repr);
    return repr;
  }

  // give up.
  return NULL;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMParaViewPipelineControllerWithRendering::Hide(
  vtkSMSourceProxy* producer, int outputPort, vtkSMViewProxy* view)
{
  if (producer == NULL || static_cast<int>(producer->GetNumberOfOutputPorts()) <= outputPort)
  {
    vtkErrorMacro("Invalid producer (" << producer << ") or outputPort (" << outputPort << ")");
    return NULL;
  }
  if (view == NULL)
  {
    // already hidden, I guess :).
    return NULL;
  }

  SM_SCOPED_TRACE(Hide).arg("producer", producer).arg("port", outputPort).arg("view", view);

  // find is there's already a representation in this view.
  if (vtkSMProxy* repr = view->FindRepresentation(producer, outputPort))
  {
    this->Hide(repr, view);
    return repr;
  }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkSMParaViewPipelineControllerWithRendering::Hide(vtkSMProxy* repr, vtkSMViewProxy* view)
{
  if (repr)
  {
    vtkSMPropertyHelper(repr, "Visibility").Set(0);
    repr->UpdateVTKObjects();

    if (vtkSMParaViewPipelineControllerWithRendering::HideScalarBarOnHide)
    {
      vtkSMPVRepresentationProxy::HideScalarBarIfNotNeeded(repr, view);
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMParaViewPipelineControllerWithRendering::HideAll(vtkSMViewProxy* view)
{
  if (view == NULL)
  {
    return;
  }

  SM_SCOPED_TRACE(CallFunction).arg("HideAll").arg(view);

  vtkSMPropertyHelper helper(view, "Representations");
  for (unsigned int i = 0; i < helper.GetNumberOfElements(); i++)
  {
    vtkSMProxy* repr = helper.GetAsProxy(i);
    vtkSMProperty* input = repr->GetProperty("Input");
    if (input)
    {
      this->Hide(repr, view);
    }
  }
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineControllerWithRendering::GetVisibility(
  vtkSMSourceProxy* producer, int outputPort, vtkSMViewProxy* view)
{
  if (producer == NULL || static_cast<int>(producer->GetNumberOfOutputPorts()) <= outputPort)
  {
    return false;
  }
  if (view == NULL)
  {
    return false;
  }
  if (vtkSMRepresentationProxy* repr = view->FindRepresentation(producer, outputPort))
  {
    return vtkSMPropertyHelper(repr, "Visibility").GetAsInt() != 0;
  }
  return false;
}

//----------------------------------------------------------------------------
vtkSMViewProxy* vtkSMParaViewPipelineControllerWithRendering::ShowInPreferredView(
  vtkSMSourceProxy* producer, int outputPort, vtkSMViewProxy* view)
{
  if (producer == NULL || static_cast<int>(producer->GetNumberOfOutputPorts()) <= outputPort)
  {
    vtkErrorMacro("Invalid producer (" << producer << ") or outputPort (" << outputPort << ")");
    return NULL;
  }

  if (strcmp(producer->GetXMLGroup(), "insitu_writer_parameters") == 0)
  {
    // This is a proxy for the Catalyst writers which isn't a real filter
    // but a placeholder for a writer during the Catalyst script export
    // process. We don't need to do anything with the views.
    return NULL;
  }

  this->UpdatePipelineBeforeDisplay(producer, outputPort, view);

  vtkSMSessionProxyManager* pxm = producer->GetSessionProxyManager();
  if (const char* preferredViewType = this->GetPreferredViewType(producer, outputPort))
  {
    if ( // no view is specified.
      view == NULL ||
      // the "view" is not of the preferred type.
      strcmp(preferredViewType, view->GetXMLName()) != 0)
    {
      vtkSmartPointer<vtkSMProxy> targetView;
      targetView.TakeReference(pxm->NewProxy("views", preferredViewType));
      if (vtkSMViewProxy* preferredView = vtkSMViewProxy::SafeDownCast(targetView))
      {
        this->InitializeProxy(preferredView);
        this->RegisterViewProxy(preferredView);
        if (this->Show(producer, outputPort, preferredView) == NULL)
        {
          vtkErrorMacro("Data cannot be shown in the preferred view!!");
          return NULL;
        }
        return preferredView;
      }
      else
      {
        vtkErrorMacro("Failed to create preferred view (" << preferredViewType << ")");
        return NULL;
      }
    }
    else
    {
      assert(view);
      if (!this->Show(producer, outputPort, view))
      {
        vtkErrorMacro("Data cannot be shown in the preferred view!!");
        return NULL;
      }
      return view;
    }
  }
  else if (view)
  {
    // If there's no preferred view, check if active view can show the data. If
    // so, show it in that view.
    if (view->CanDisplayData(producer, outputPort))
    {
      if (this->Show(producer, outputPort, view))
      {
        return view;
      }

      vtkErrorMacro("View should have been able to show the data, "
                    "but it failed to do so. This may point to a development bug.");
      return NULL;
    }
  }

  // No preferred view is found and "view" is NULL or cannot show the data.
  // ParaView's default behavior is to create a render view, in that case, if the
  // render view can show the data.
  if (view == NULL || strcmp(view->GetXMLName(), "RenderView") != 0)
  {
    vtkSmartPointer<vtkSMProxy> renderView;
    renderView.TakeReference(pxm->NewProxy("views", "RenderView"));
    if (vtkSMViewProxy* preferredView = vtkSMViewProxy::SafeDownCast(renderView))
    {
      this->InitializeProxy(preferredView);
      this->RegisterViewProxy(preferredView);
      if (this->Show(producer, outputPort, preferredView) == NULL)
      {
        vtkErrorMacro("Data cannot be shown in the defaulted render view!!");
        return NULL;
      }
      return preferredView;
    }
  }

  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkSMParaViewPipelineControllerWithRendering::GetPreferredViewType(
  vtkSMSourceProxy* producer, int outputPort)
{
  // 1. Check if there's a hint for the producer. If so, use that.
  if (const char* viewType = vtkFindTypeFromHints(producer->GetHints(), outputPort, "View"))
  {
    return viewType;
  }

  vtkPVDataInformation* dataInfo = producer->GetDataInformation(outputPort);
  if (dataInfo->DataSetTypeIsA("vtkTable") &&
    (vtkIsOutputTypeNonStandard(producer->GetHints(), outputPort) == false))
  {
    return "SpreadSheetView";
  }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkSMParaViewPipelineControllerWithRendering::UpdatePipelineBeforeDisplay(
  vtkSMSourceProxy* producer, int outputPort, vtkSMViewProxy* view)
{
  (void)outputPort;
  if (!producer)
  {
    return;
  }

  // Update using view time, or timekeeper time.
  double time = view
    ? vtkSMPropertyHelper(view, "ViewTime").GetAsDouble()
    : vtkSMPropertyHelper(this->FindTimeKeeper(producer->GetSession()), "Time").GetAsDouble();
  producer->UpdatePipeline(time);
}

//----------------------------------------------------------------------------
template <class T>
bool vtkWriteImage(T* viewOrLayout, const char* filename, int magnification, int quality)
{
  if (!viewOrLayout)
  {
    return false;
  }
  SM_SCOPED_TRACE(SaveCameras).arg("proxy", viewOrLayout);

  SM_SCOPED_TRACE(CallFunction)
    .arg("SaveScreenshot")
    .arg(filename)
    .arg((vtkSMViewProxy::SafeDownCast(viewOrLayout) ? "view" : "layout"), viewOrLayout)
    .arg("magnification", magnification)
    .arg("quality", quality)
    .arg("comment", "save screenshot");

  vtkSmartPointer<vtkImageData> img;
  img.TakeReference(viewOrLayout->CaptureWindow(magnification));
  if (img && vtkProcessModule::GetProcessModule()->GetPartitionId() == 0)
  {
    return vtkSMUtilities::SaveImage(img.GetPointer(), filename, quality) == vtkErrorCode::NoError;
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineControllerWithRendering::WriteImage(
  vtkSMViewProxy* view, const char* filename, int magnification, int quality)
{
  return vtkWriteImage<vtkSMViewProxy>(view, filename, magnification, quality);
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineControllerWithRendering::WriteImage(
  vtkSMViewLayoutProxy* layout, const char* filename, int magnification, int quality)
{
  return vtkWriteImage<vtkSMViewLayoutProxy>(layout, filename, magnification, quality);
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineControllerWithRendering::RegisterViewProxy(
  vtkSMProxy* proxy, const char* proxyname)
{
  if (!proxy)
  {
    return false;
  }

  bool retval = this->Superclass::RegisterViewProxy(proxy, proxyname);
  if (proxy->HasAnnotation("ParaView::DetachedFromLayout") &&
    strcmp(proxy->GetAnnotation("ParaView::DetachedFromLayout"), "true") == 0)
  {
    return retval;
  }

  vtkSMSessionProxyManager* pxm = proxy->GetSessionProxyManager();

  // locate layout (create a new one if needed).
  vtkSMProxySelectionModel* selmodel = pxm->GetSelectionModel("ActiveView");
  assert(selmodel != NULL);
  vtkSMViewProxy* activeView = vtkSMViewProxy::SafeDownCast(selmodel->GetCurrentProxy());
  vtkSMProxy* activeLayout = vtkSMViewLayoutProxy::FindLayout(activeView);
  activeLayout =
    activeLayout ? activeLayout : this->FindProxy(pxm, "layouts", "misc", "ViewLayout");
  if (!activeLayout)
  {
    // no active layout is present at all. Create a new one.
    activeLayout = pxm->NewProxy("misc", "ViewLayout");
    if (activeLayout)
    {
      this->InitializeProxy(activeLayout);
      this->RegisterLayoutProxy(activeLayout);
      activeLayout->FastDelete();
    }
  }
  if (activeLayout)
  {
    vtkSMProxy* layoutAssigned =
      vtkSMViewLayoutProxy::FindLayout(vtkSMViewProxy::SafeDownCast(proxy));
    activeLayout = layoutAssigned ? layoutAssigned : activeLayout;
    vtkSMViewLayoutProxy::SafeDownCast(activeLayout)
      ->AssignViewToAnyCell(vtkSMViewProxy::SafeDownCast(proxy), 0);
  }
  return retval;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineControllerWithRendering::RegisterLayoutProxy(
  vtkSMProxy* proxy, const char* proxyname)
{
  if (!proxy)
  {
    return false;
  }

  SM_SCOPED_TRACE(RegisterLayoutProxy).arg("layout", proxy);

  vtkSMSessionProxyManager* pxm = proxy->GetSessionProxyManager();
  pxm->RegisterProxy("layouts", proxyname, proxy);
  return true;
}

//----------------------------------------------------------------------------
void vtkSMParaViewPipelineControllerWithRendering::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
