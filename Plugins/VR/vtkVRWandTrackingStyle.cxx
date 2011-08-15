#include "vtkVRWandTrackingStyle.h"

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

vtkVRWandTrackingStyle::vtkVRWandTrackingStyle(QObject* parentObject) :
  Superclass(parentObject)
{

}

vtkVRWandTrackingStyle::~vtkVRWandTrackingStyle()
{

}

// ----------------------------------------------------------------------------
// This handler currently only get the
bool vtkVRWandTrackingStyle::handleEvent(const vtkVREventData& data)
{
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
void vtkVRWandTrackingStyle::HandleTracker( const vtkVREventData& data )
{
  if ( this->Name == QString(data.name.c_str()) ) // Handle wand tracking
    {
    this->SetWandPoseProperty( data );
    }
}

// ----------------------------------------------------------------------------
void vtkVRWandTrackingStyle::HandleButton( const vtkVREventData& data )
{
}

// ----------------------------------------------------------------------------
void vtkVRWandTrackingStyle::HandleAnalog( const vtkVREventData& data )
{
}

// ----------------------------------------------------------------------------
bool vtkVRWandTrackingStyle::GetWandPoseProxyNProperty( vtkSMRenderViewProxy** outProxy,
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
      prop = vtkSMDoubleVectorProperty::SafeDownCast(proxy->GetProperty( "WandPose" ) );
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

bool vtkVRWandTrackingStyle::SetWandPoseProperty(const vtkVREventData &data)
{
  vtkSMRenderViewProxy *proxy =0;
  vtkSMDoubleVectorProperty *prop =0;
  if ( this->GetWandPoseProxyNProperty( &proxy, &prop ) )
    {
    prop->SetElement( 0,  data.data.tracker.matrix[0] );
    prop->SetElement( 1,  data.data.tracker.matrix[1] );
    prop->SetElement( 2,  data.data.tracker.matrix[2] );
    prop->SetElement( 3,  data.data.tracker.matrix[3]  );

    prop->SetElement( 4,  data.data.tracker.matrix[4] );
    prop->SetElement( 5,  data.data.tracker.matrix[5] );
    prop->SetElement( 6,  data.data.tracker.matrix[6] );
    prop->SetElement( 7,  data.data.tracker.matrix[7]  );

    prop->SetElement( 8,  data.data.tracker.matrix[8] );
    prop->SetElement( 9,  data.data.tracker.matrix[9] );
    prop->SetElement( 10, data.data.tracker.matrix[10] );
    prop->SetElement( 11, data.data.tracker.matrix[11]  );

    prop->SetElement( 12, 0.0 );
    prop->SetElement( 13, 0.0 );
    prop->SetElement( 14, 0.0 );
    prop->SetElement( 15, 1.0 );

    return true;
    }
  return false;
}

bool vtkVRWandTrackingStyle::UpdateNRenderWithWandPose()
{
  vtkSMRenderViewProxy *proxy =0;
  vtkSMDoubleVectorProperty *prop =0;
  if ( GetWandPoseProxyNProperty( &proxy, &prop ) )
    {
    proxy->UpdateVTKObjects();
    proxy->StillRender();
    return true;
    }
  return false;
}

bool vtkVRWandTrackingStyle::update()
{
    // Update the when all the events are handled
    this->UpdateNRenderWithWandPose();
    return false;
}
