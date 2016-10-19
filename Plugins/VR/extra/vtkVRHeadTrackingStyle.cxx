#include "vtkVRHeadTrackingStyle.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "pqView.h"
#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkVRQueue.h"

vtkVRHeadTrackingStyle::vtkVRHeadTrackingStyle(QObject* parentObject)
  : Superclass(parentObject)
{
  this->Proxy = 0;
  this->Property = 0;
  this->IsFoundProxyProperty = GetHeadPoseProxyNProperty();
}

vtkVRHeadTrackingStyle::~vtkVRHeadTrackingStyle()
{
}

// ----------------------------------------------------------------------------
bool vtkVRHeadTrackingStyle::handleEvent(const vtkVREventData& data)
{
  // std::cout<< "Event Name = " << data.name << std::endl;
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
void vtkVRHeadTrackingStyle::HandleTracker(const vtkVREventData& data)
{
  if (this->Name == QString(data.name.c_str()))
  {
    this->SetHeadPoseProperty(data);
  }
}

// ----------------------------------------------------------------------------
void vtkVRHeadTrackingStyle::HandleButton(const vtkVREventData& data)
{
}

// ----------------------------------------------------------------------------
void vtkVRHeadTrackingStyle::HandleAnalog(const vtkVREventData& data)
{
}

// ----------------------------------------------------------------------------
bool vtkVRHeadTrackingStyle::GetHeadPoseProxyNProperty()
{
  pqView* view = 0;
  view = pqActiveObjects::instance().activeView();
  if (view)
  {
    this->Proxy = vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
    if (this->Proxy)
    {
      this->Property =
        vtkSMDoubleVectorProperty::SafeDownCast(this->Proxy->GetProperty("EyeTransformMatrix"));
      if (this->Property)
      {
        return true;
      }
    }
  }
  return false;
}

// ----------------------------------------------------------------------------
bool vtkVRHeadTrackingStyle::SetHeadPoseProperty(const vtkVREventData& data)
{
  if (!this->IsFoundProxyProperty)
  {
    this->IsFoundProxyProperty = GetHeadPoseProxyNProperty();
    return false;
  }
  for (int i = 0; i < 16; ++i)
  {
    this->Property->SetElement(i, data.data.tracker.matrix[i]);
  }
  return true;
}

// ----------------------------------------------------------------------------
bool vtkVRHeadTrackingStyle::update()
{
  if (!this->IsFoundProxyProperty)
  {
    this->IsFoundProxyProperty = GetHeadPoseProxyNProperty();
    return false;
  }
  this->Proxy->UpdateVTKObjects();
  this->Proxy->StillRender();
  return false;
}
