#include "vtkVRActiveObjectManipulationStyle.h"

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

vtkVRActiveObjectManipulationStyle::vtkVRActiveObjectManipulationStyle(QObject* parentObject) :
  Superclass(parentObject)
{

}

vtkVRActiveObjectManipulationStyle::~vtkVRActiveObjectManipulationStyle()
{

}

// ----------------------------------------------------------------------------
// This handler currently only get the
bool vtkVRActiveObjectManipulationStyle::handleEvent(const vtkVREventData& data)
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
void vtkVRActiveObjectManipulationStyle::HandleTracker( const vtkVREventData& data )
{
}

// ----------------------------------------------------------------------------
void vtkVRActiveObjectManipulationStyle::HandleButton( const vtkVREventData& data )
{
  std::cout << "(Button" << "\n"
            << "  :from  " << data.name <<"\n"
            << "  :time  " << data.timeStamp << "\n"
            << "  :id    " << data.data.button.button << "\n"
            << "  :state " << data.data.button.state << " )" << "\n";

}

// ----------------------------------------------------------------------------
void vtkVRActiveObjectManipulationStyle::HandleAnalog( const vtkVREventData& data )
{
  std::cout << "(Analog" << "\n"
            << "  :from  " << data.name <<"\n"
            << "  :time  " << data.timeStamp << "\n"
            << "  :channel '(" ;
  for ( int i =0 ; i<data.data.analog.num_channel; i++ )
    {
    std::cout << data.data.analog.channel[i] << " ";
    }
  std::cout  << " ))" << "\n" ;
  HandleSpaceNavigatorAnalog(data);
}

// ----------------------------------------------------------------------------
bool vtkVRActiveObjectManipulationStyle::GetHeadPoseProxyNProperty( vtkSMRenderViewProxy** outProxy,
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

bool vtkVRActiveObjectManipulationStyle::SetHeadPoseProperty(const vtkVREventData &data)
{
  vtkSMRenderViewProxy *proxy =0;
  vtkSMDoubleVectorProperty *prop =0;
  if ( this->GetHeadPoseProxyNProperty( &proxy, &prop ) )
    {
    double rotMat[3][3];
    vtkMath::QuaternionToMatrix3x3( data.data.tracker.quat, rotMat );

    prop->SetElement( 0,  rotMat[0][0] );
    prop->SetElement( 1,  rotMat[0][1] );
    prop->SetElement( 2,  rotMat[0][2] );
    prop->SetElement( 3,  data.data.tracker.pos [0]*-1  );

    prop->SetElement( 4,  rotMat[1][0] );
    prop->SetElement( 5,  -rotMat[1][1] );
    prop->SetElement( 6,  rotMat[1][2] );
    prop->SetElement( 7,  data.data.tracker.pos [1]*1  );

    prop->SetElement( 8,  rotMat[2][0] );
    prop->SetElement( 9,  rotMat[2][1] );
    prop->SetElement( 10, rotMat[2][2] );
    prop->SetElement( 11, data.data.tracker.pos [2]*1  );

    prop->SetElement( 12, 0.0 );
    prop->SetElement( 13, 0.0 );
    prop->SetElement( 14, 0.0 );
    prop->SetElement( 15, 1.0 );

    return true;
    }
  return false;
}

bool vtkVRActiveObjectManipulationStyle::UpdateNRenderWithHeadPose()
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

// -------------------------------------------------------------------------fun
vtkAnalog AugmentChannelsToRetainLargestMagnitude(const vtkAnalog t)
{
  vtkAnalog at;
  // Make a list of the magnitudes into at
  for(int i=0;i<6;++i)
    {
      if(t.channel[i] < 0.0)
        at.channel[i] = t.channel[i]*-1;
      else
        at.channel[i]= t.channel[i];
    }

  // Get the max value;
  int max =0;
  for(int i=1;i<6;++i)
    {
      if(at.channel[i] > at.channel[max])
          max = i;
    }

  // copy the max value of t into at (rest are 0)
  for (int i = 0; i < 6; ++i)
    {
      (i==max)?at.channel[i]=t.channel[i]:at.channel[i]=0.0;
    }
  return at;
}

void vtkVRActiveObjectManipulationStyle::HandleSpaceNavigatorAnalog( const vtkVREventData& data )
{
  vtkAnalog at = AugmentChannelsToRetainLargestMagnitude(data.data.analog);
  // printf("%6.3f, %6.3f, %6.3f, %6.3f, %6.3f, %6.3f\n", at.channel[0],
  //    at.channel[1], at.channel[2], at.channel[3], at.channel[4],
  //    at.channel[5]);
  // Apply up-down motion

  pqView *view = 0;
  pqDataRepresentation *rep =0;


  view = pqActiveObjects::instance().activeView();
  rep = pqActiveObjects::instance().activeRepresentation();

  if(rep)
    {
    vtkCamera* camera;
    double pos[3], up[3], dir[3];
    double orient[3];

    vtkSMRenderViewProxy *viewProxy = 0;
    vtkSMRepresentationProxy *repProxy = 0;
    viewProxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );
    repProxy = vtkSMRepresentationProxy::SafeDownCast(rep->getProxy());

    if ( repProxy && viewProxy )
      {
      vtkSMPropertyHelper(repProxy,"Position").Get(pos,3);
      vtkSMPropertyHelper(repProxy,"Orientation").Get(orient,3);
      camera = viewProxy->GetActiveCamera();
      camera->GetDirectionOfProjection(dir);
      camera->OrthogonalizeViewUp();
      camera->GetViewUp(up);

      for (int i = 0; i < 3; i++)
        {
        double dx = -0.01*at.channel[2]*up[i];
        pos[i] += dx;
        }

      double r[3];
      vtkMath::Cross(dir, up, r);

      for (int i = 0; i < 3; i++)
        {
        double dx = 0.01*at.channel[0]*r[i];
        pos[i] += dx;
        }

      for(int i=0;i<3;++i)
        {
        double dx = -0.01*at.channel[1]*dir[i];
        pos[i] +=dx;
        }

      // pos[0] += at.channel[0];
      // pos[1] += at.channel[1];
      // pos[2] += at.channel[2];
      orient[0] += 4.0*at.channel[3];
      orient[1] += 4.0*at.channel[5];
      orient[2] += 4.0*at.channel[4];
      vtkSMPropertyHelper(repProxy,"Position").Set(pos,3);
      vtkSMPropertyHelper(repProxy,"Orientation").Set(orient,3);
      repProxy->UpdateVTKObjects();
      }
    }
  else if ( view )
    {
    vtkSMRenderViewProxy *viewProxy = 0;
    viewProxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );

    if ( viewProxy )
      {
      vtkCamera* camera;
      double pos[3], fp[3], up[3], dir[3];

      camera = viewProxy->GetActiveCamera();
      camera->GetPosition(pos);
      camera->GetFocalPoint(fp);
      camera->GetDirectionOfProjection(dir);
      camera->OrthogonalizeViewUp();
      camera->GetViewUp(up);

      for (int i = 0; i < 3; i++)
        {
        double dx = 0.01*at.channel[2]*up[i];
        pos[i] += dx;
        fp[i]  += dx;
        }

      // Apply right-left motion
      double r[3];
      vtkMath::Cross(dir, up, r);

      for (int i = 0; i < 3; i++)
        {
        double dx = -0.01*at.channel[0]*r[i];
        pos[i] += dx;
        fp[i]  += dx;
        }

      camera->SetPosition(pos);
      camera->SetFocalPoint(fp);

      camera->Dolly(pow(1.01,at.channel[1]));
      camera->Elevation(  4.0*at.channel[3]);
      camera->Azimuth(    4.0*at.channel[5]);
      camera->Roll(       4.0*at.channel[4]);

      // viewProxy->GetRenderer()->ResetCameraClippingRange();
      // viewProxy->GetRenderWindow()->Render();
      }
    }
}

bool vtkVRActiveObjectManipulationStyle::update()
{
    // Update the when all the events are handled
    this->UpdateNRenderWithHeadPose();
    return false;
}
