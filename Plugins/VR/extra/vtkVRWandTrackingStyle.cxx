#include "vtkVRWandTrackingStyle.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "pqView.h"
#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkVRQueue.h"

vtkVRWandTrackingStyle::vtkVRWandTrackingStyle(QObject* parentObject)
  : Superclass(parentObject)
{
  this->Proxy = 0;
  this->Property = 0;
  this->IsFoundProxyProperty = GetWandPoseProxyNProperty();
}

vtkVRWandTrackingStyle::~vtkVRWandTrackingStyle()
{
}

// ----------------------------------------------------------------------------
bool vtkVRWandTrackingStyle::handleEvent(const vtkVREventData& data)
{
  switch (data.eventType)
  {
    case BUTTON_EVENT:
      this->HandleButton(data);
      break;
    case ANALOG_EVENT:
      this->HandleAnalog(data);
      break;
    case TRACKER_EVENT:
      this->HandleTracker(data);
      break;
  }
  return false;
}

// ----------------------------------------------------------------------------
void vtkVRWandTrackingStyle::HandleTracker(const vtkVREventData& data)
{
  if (this->Name == QString(data.name.c_str())) // Handle wand tracking
  {
    this->SetWandPoseProperty(data);
  }
}

// ----------------------------------------------------------------------------
void vtkVRWandTrackingStyle::HandleButton(const vtkVREventData& data)
{
}

// ----------------------------------------------------------------------------
void vtkVRWandTrackingStyle::HandleAnalog(const vtkVREventData& data)
{
}

// ----------------------------------------------------------------------------
bool vtkVRWandTrackingStyle::GetWandPoseProxyNProperty()
{
  pqView* view = 0;
  view = pqActiveObjects::instance().activeView();
  if (view)
  {
    this->Proxy = vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
    if (this->Proxy)
    {
      this->Property =
        vtkSMDoubleVectorProperty::SafeDownCast(this->Proxy->GetProperty("ModelTransformMatrix"));
      if (this->Property)
      {
        return true;
      }
    }
  }
  return false;
}

// ----------------------------------------------------------------------------
bool vtkVRWandTrackingStyle::SetWandPoseProperty(const vtkVREventData& data)
{
  if (!this->IsFoundProxyProperty)
  {
    this->IsFoundProxyProperty = GetWandPoseProxyNProperty();
    return false;
  }

  for (int i = 0; i < 16; ++i)
  {
    this->Property->SetElement(i, data.data.tracker.matrix[i]);
  }
  return true;
}

// ----------------------------------------------------------------------------
bool vtkVRWandTrackingStyle::update()
{
  if (!this->IsFoundProxyProperty)
  {
    this->IsFoundProxyProperty = GetWandPoseProxyNProperty();
    return false;
  }
  this->Proxy->UpdateVTKObjects();
  this->Proxy->StillRender();
  return false;
}
