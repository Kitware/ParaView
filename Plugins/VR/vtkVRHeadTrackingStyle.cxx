#include "vtkVRHeadTrackingStyle.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "pqView.h"
#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkVRQueue.h"

vtkVRHeadTrackingStyle::vtkVRHeadTrackingStyle(QObject* parentObject) :
  Superclass(parentObject)
{

}

vtkVRHeadTrackingStyle::~vtkVRHeadTrackingStyle()
{

}

// ----------------------------------------------------------------------------
// This handler currently only get the
bool vtkVRHeadTrackingStyle::handleEvent(const vtkVREventData& data)
{
  // std::cout<< "Event Name = " << data.name << std::endl;
  switch( data.eventType )
    {
  case BUTTON_EVENT:
    this->HandleButton( data );
    break;
  case ANALOG_EVENT:
    this->HandleAnalog( data );
    break;
  case TRACKER_EVENT:
    this->HandleTracker( data );
    break;
    }
  return false;
}

// ----------------------------------------------------------------------------
void vtkVRHeadTrackingStyle::HandleTracker( const vtkVREventData& data )
{
  if ( this->Name == QString(data.name.c_str()) ) // Handle head tracking
    {
    this->SetHeadPoseProperty( data );
    }
}

// ----------------------------------------------------------------------------
void vtkVRHeadTrackingStyle::HandleButton( const vtkVREventData& data )
{
}

// ----------------------------------------------------------------------------
void vtkVRHeadTrackingStyle::HandleAnalog( const vtkVREventData& data )
{
}

// ----------------------------------------------------------------------------
bool vtkVRHeadTrackingStyle::GetHeadPoseProxyNProperty( vtkSMRenderViewProxy** outProxy,
                                                   vtkSMDoubleVectorProperty** outProp)
{
  *outProxy =0;
  *outProp = 0;
  vtkSMRenderViewProxy *proxy =0;
  vtkSMDoubleVectorProperty *prop =0;

  pqView *view = 0;
  view = pqActiveObjects::instance().activeView();
  if ( view )
    {
    proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );
    if ( proxy )
      {
      prop = vtkSMDoubleVectorProperty::SafeDownCast(proxy->GetProperty( "HeadPose" ) );
      if ( prop )
        {
        *outProxy = proxy;
        *outProp =  prop;
        return true;
        }
      }
    }
  return false;
}

bool vtkVRHeadTrackingStyle::SetHeadPoseProperty(const vtkVREventData &data)
{
  vtkSMRenderViewProxy *proxy =0;
  vtkSMDoubleVectorProperty *prop =0;
  if ( this->GetHeadPoseProxyNProperty( &proxy, &prop ) )
    {
    for (int i = 0; i < 16; ++i)
      {
      prop->SetElement( i, data.data.tracker.matrix[i] );
      }
    return true;
    }
  return false;
}

bool vtkVRHeadTrackingStyle::UpdateNRenderWithHeadPose()
{
  vtkSMRenderViewProxy *proxy =0;
  vtkSMDoubleVectorProperty *prop =0;
  if ( GetHeadPoseProxyNProperty( &proxy, &prop ) )
    {
    proxy->UpdateVTKObjects();
    proxy->StillRender();
    return true;
    }
  return false;
}

bool vtkVRHeadTrackingStyle::update()
{
    this->UpdateNRenderWithHeadPose();
    return false;
}
