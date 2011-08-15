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
}

// ----------------------------------------------------------------------------
void vtkVRActiveObjectManipulationStyle::HandleAnalog( const vtkVREventData& data )
{
  HandleSpaceNavigatorAnalog(data);
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
      }
    }
}

bool vtkVRActiveObjectManipulationStyle::update()
{
  vtkSMRenderViewProxy *proxy =0;
  pqView *view = 0;
  view = pqActiveObjects::instance().activeView();
  if ( view )
    {
    proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );
    if ( proxy )
      {
      proxy->UpdateVTKObjects();
      proxy->StillRender();
      }
    }
  return false;
}
