// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMVRTrackStyleProxy.h"

#include "vtkCamera.h"
#include "vtkCollection.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkStringFormatter.h"
#include "vtkStringScanner.h"
#include "vtkVRQueue.h"

#include <algorithm>
#include <map>
#include <sstream>

namespace
{
class Viewer
{
public:
  vtkNew<vtkMatrix4x4> TrackerMatrix;
  double EyeSeparation;
};
}

class vtkSMVRTrackStyleProxy::Internal
{
public:
  std::map<int, Viewer> Viewers;
  std::vector<double> EyeTransforms;
  std::vector<double> EyeSeparations;
  bool InCAVEMode = true;
  std::string RenderViewMatrixProperty;
  std::string RenderViewEyeSepProperty;
};

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRTrackStyleProxy);

// ----------------------------------------------------------------------------
vtkSMVRTrackStyleProxy::vtkSMVRTrackStyleProxy()
  : Superclass()
  , Internals(new vtkSMVRTrackStyleProxy::Internal())
{
}

// ----------------------------------------------------------------------------
void vtkSMVRTrackStyleProxy::SetControlledProxy(vtkSMProxy* proxy)
{
  this->Superclass::SetControlledProxy(proxy);

  vtkSMRenderViewProxy* rvProxy = vtkSMRenderViewProxy::SafeDownCast(proxy);
  if (rvProxy)
  {
    this->ClearAllRoles();

    vtkSMDoubleVectorProperty* dvp =
      vtkSMDoubleVectorProperty::SafeDownCast(this->GetProperty("EyeSeparation"));

    if (rvProxy->GetIsInCAVE())
    {
      // In CAVE mode, the render view proxy exposes methods that retrieve the display
      // configuration from the server (initialized from pvx). Query that here to get
      // the initial value to set on the property.
      int numViewers = rvProxy->GetNumberOfViewers();
      this->Internals->InCAVEMode = true;
      this->Internals->RenderViewMatrixProperty = "EyeTransforms";
      this->Internals->RenderViewEyeSepProperty = "EyeSeparations";
      this->Internals->EyeTransforms.resize(16 * numViewers);
      this->Internals->EyeSeparations.resize(numViewers);

      dvp->SetNumberOfElements(numViewers);

      // Use the per-display eye separation api to get a distinct default eye separation
      // for each viewer.
      for (int i = 0; i < numViewers; ++i)
      {
        int viewerId = rvProxy->GetId(i);
        std::string role("Viewer");
        role += vtk::to_string(viewerId);
        this->AddTrackerRole(role);
        auto& viewer = this->Internals->Viewers[viewerId];
        viewer.EyeSeparation = rvProxy->GetEyeSeparation(i);
        dvp->SetElement(i, viewer.EyeSeparation);
      }
    }
    else
    {
      // Not in CAVE mode
      this->Internals->InCAVEMode = false;
      this->Internals->RenderViewMatrixProperty = "EyeTransformMatrix";
      this->Internals->RenderViewEyeSepProperty = "EyeSeparation";
      this->Internals->EyeTransforms.resize(16);
      this->Internals->EyeSeparations.resize(1);
      this->AddTrackerRole("Viewer0");
      dvp->SetNumberOfElements(1);
      auto& viewer = this->Internals->Viewers[0];
      viewer.EyeSeparation = rvProxy->GetActiveCamera()->GetEyeSeparation();
      dvp->SetElement(0, viewer.EyeSeparation);
    }
  }
}

// ----------------------------------------------------------------------------
void vtkSMVRTrackStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkSMVRTrackStyleProxy::Update()
{
  if (this->ControlledProxy != nullptr)
  {
    vtkSMRenderViewProxy* rvProxy = vtkSMRenderViewProxy::SafeDownCast(this->ControlledProxy);
    if (rvProxy)
    {
      int numViewers = this->Internals->Viewers.size();
      for (int i = 0; i < numViewers; ++i)
      {
        double* data = this->Internals->Viewers[i].TrackerMatrix->GetData();
        for (int j = 0; j < 16; ++j)
        {
          this->Internals->EyeTransforms[i * 16 + j] = data[j];
        }

        this->Internals->EyeSeparations[i] = this->Internals->Viewers[i].EyeSeparation;
      }

      vtkSMPropertyHelper(this->ControlledProxy, this->Internals->RenderViewMatrixProperty.c_str())
        .Set(this->Internals->EyeTransforms.data(), 16 * numViewers);

      vtkSMPropertyHelper(this->ControlledProxy, this->Internals->RenderViewEyeSepProperty.c_str())
        .Set(this->Internals->EyeSeparations.data(), numViewers);

      this->ControlledProxy->UpdateVTKObjects();
    }
  }

  return true;
}

// ----------------------------------------------------------------------------
void vtkSMVRTrackStyleProxy::UpdateVTKObjects()
{
  // Get and store updated eye separations from the UI locally
  vtkSMDoubleVectorProperty* dvp;
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(this->GetProperty("EyeSeparation"));
  unsigned int numElts = dvp->GetNumberOfElements();

  for (unsigned int idx = 0; idx < numElts; ++idx)
  {
    this->Internals->Viewers[idx].EyeSeparation = dvp->GetElement(idx);
  }

  // Let the eye separations be applied in the Update() method

  this->Superclass::UpdateVTKObjects();
}

// ----------------------------------------------------------------------------
void vtkSMVRTrackStyleProxy::HandleTracker(const vtkVREvent& event)
{
  std::string role = this->GetTrackerRole(event.name);

  if (role.empty())
  {
    return;
  }

  // We generated all the role names to start with "Viewer" followed immediately
  // by the viewer id, so this should be safe.
  std::string_view vId = std::string_view(role).substr(6);
  int viewerId = vtk::scan_int<int>(vId)->value();

  if (this->Internals->Viewers.count(viewerId) == 0)
  {
    return;
  }

  auto& viewer = this->Internals->Viewers[viewerId];
  viewer.TrackerMatrix->DeepCopy(event.data.tracker.matrix);
}

// ----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMVRTrackStyleProxy::SaveConfiguration()
{
  vtkPVXMLElement* elt = Superclass::SaveConfiguration();
  this->SaveXMLState(elt);
  return elt;
}

// ----------------------------------------------------------------------------
bool vtkSMVRTrackStyleProxy::Configure(vtkPVXMLElement* child, vtkSMProxyLocator* locator)
{
  bool result = Superclass::Configure(child, locator);

  if (this->GetControlledProxy() == nullptr)
  {
    vtkErrorMacro(<< "Unable to load vtkSMVRTrackStyleProxy from state, see warnings printed "
                  << "above for details.");
    return false;
  }

  // The actual number of viewers is updated in SetControlledProxy(), and comes from the pvx
  // file via the vtkSMRenderViewProxy. Compare the actual number of viewers with the number
  // of trackers stored in the state file.
  vtkNew<vtkCollection> xmlTrackers;
  child->FindNestedElementByName("Tracker", xmlTrackers);
  size_t numTrackersInState = static_cast<size_t>(xmlTrackers->GetNumberOfItems());
  size_t numConfiguredViewers = this->Trackers.size();

  if (numConfiguredViewers > numTrackersInState)
  {
    // Not enough trackers/properties stored in state to cover the actual number of
    // independent viewers. Do not attempt to load the properties, because if we do,
    // there will be fewer eye separation fields than needed/supported. Allow the
    // proxy to be created though, as it should work fine after manual configuration.
    vtkWarningMacro(<< "State file lists " << numTrackersInState << " Tracker elements, which "
                    << "is less than the " << numConfiguredViewers << " configured Tracker roles. "
                    << " Proxy properties will not be loaded from state to avoid further issues.");
  }
  else if (numConfiguredViewers < numTrackersInState)
  {
    // Extra trackers/properties in state file, compared to actual number of independent
    // viewers. Warnings were printed about the extra tracker/role bindings, but otherwise
    // those have been ignored. Do not attempt to load the properties, because if we do,
    // there will be more eye separation fields than needed/supported.
    vtkWarningMacro(<< "State file lists " << numTrackersInState << " Tracker elements, which "
                    << "is more than the " << numConfiguredViewers << " configured Tracker roles. "
                    << "Extra Tracker elements have been ignored, and Proxy properties will not "
                    << "be loaded from state to avoid further issues.");
  }
  else
  {
    vtkPVXMLElement* proxyChild = child->FindNestedElementByName("Proxy");
    if (proxyChild)
    {
      if (this->LoadXMLState(proxyChild, locator) > 0)
      {
        this->UpdateVTKObjects();
      }
      else
      {
        vtkWarningMacro(<< "Encountered an error loading proxy properties from state, "
                        << "vtkSMVRInteractorStyle may not function as expected. "
                        << "See warnings printed above for details.");
      }
    }
  }

  if (!result)
  {
    vtkWarningMacro(<< "There was a problem loading vtkSMVRTrackStyleProxy from state, and it "
                    << "may not function as expected without manual configuration. See warnings "
                    << "printed above for details.");
  }

  return true;
}
