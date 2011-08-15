/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVRPNCallBackHandlers.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVRPNCallBackHandlers.h"

#include <vrpn_Tracker.h>
#include <vrpn_Button.h>
#include <vrpn_Analog.h>
#include <vrpn_Dial.h>
#include <vrpn_Text.h>
#include "vtkMath.h"
#include "pqActiveObjects.h"
#include "pqView.h"
#include <pqDataRepresentation.h>
#include "vtkSMRenderViewProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include "vtkVRPNConnection.h"

#include <vtkstd/vector>
#include <iostream>

// -------------------------------------------------------------------------fun
void VRPN_CALLBACK handleAnalogChange(void* userdata, const vrpn_ANALOGCB b)
{
  vtkVRPNConnection *self = static_cast<vtkVRPNConnection*> ( userdata );
  self->NewAnalogValue( b );
}

// -------------------------------------------------------------------------fun
void VRPN_CALLBACK handleButtonChange(void* userdata, vrpn_BUTTONCB b)
{
  vtkVRPNConnection *self = static_cast<vtkVRPNConnection*> ( userdata );
  self->NewButtonValue( b );
}

// -------------------------------------------------------------------------fun
void VRPN_CALLBACK handleTrackerChange(void *userdata, const vrpn_TRACKERCB t)
{
  //  printf( " inside trackin handler\n" );
  vtkVRPNConnection *self = static_cast<vtkVRPNConnection*> ( userdata );
  self->NewTrackerValue( t );
}

#if 0
// -------------------------------------------------------------------------fun
void VRPN_CALLBACK handleTrackerPosQuat(void *userdata,
                                        const vrpn_TRACKERCB t)
{
  t_user_callback *tData=static_cast<t_user_callback *>(userdata);

  // Make sure we have a count value for this sensor
  while(tData->t_counts.size() <= static_cast<unsigned>(t.sensor))
    {
    printf( "VRPN_CALLBACK::handleTrackerPosQuat (%d , %d)\n",tData->t_counts.size(),t.sensor );
    tData->t_counts.push_back(0);
    }

  // See if we have gotten enough reports from this sensor that we should
  // print this one.  If so, print and reset the count.
  const unsigned tracker_stride = 15;    // Every nth report will be printed

  if ( ++tData->t_counts[t.sensor] >= tracker_stride )
    {
    tData->t_counts[t.sensor] = 0;
    if(t.sensor >0)
      {
      return;
      }

    pqView *view = 0;
    view = pqActiveObjects::instance().activeView();


    if ( view )
      {
      vtkSMRenderViewProxy *proxy = 0;
      proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );
      // proxy = view->GetProxyManager()->GetProxy( "RenderView" );


      if ( proxy )
        {
        double invRotMat[3][3];
        double rotMat[3][3];
        vtkMath::QuaternionToMatrix3x3(t.quat,rotMat);
        //vtkMath::Invert3x3( invRotMat, rotMat );
        //vtkMath::Transpose3x3( invRotMat, rotMat );
        vtkSMDoubleVectorProperty *prop = 0;
        prop = vtkSMDoubleVectorProperty::SafeDownCast(proxy->GetProperty( "HeadPose" ) );
        if ( prop )
          {
          prop->SetElement( 0,  rotMat[0][0] );
          prop->SetElement( 1,  rotMat[0][1] );
          prop->SetElement( 2,  rotMat[0][2] );
          prop->SetElement( 3,  t.pos [0]*1  );

          prop->SetElement( 4,  rotMat[1][0] );
          prop->SetElement( 5,  rotMat[1][1] );
          prop->SetElement( 6,  rotMat[1][2] );
          prop->SetElement( 7,  t.pos [1]*1  );

          prop->SetElement( 8,  rotMat[2][0] );
          prop->SetElement( 9,  rotMat[2][1] );
          prop->SetElement( 10, rotMat[2][2] );
          prop->SetElement( 11, t.pos [2]*1  );

          prop->SetElement( 12, 0.0 );
          prop->SetElement( 13, 0.0 );
          prop->SetElement( 14, 0.0 );
          prop->SetElement( 15, 1.0 );

          float zero = 0.0f;
          float one = 1.0f;
          // printf ( "\n*******************************************************************************\n" );
          // printf( "| %3.3f %3.3f %3.3f %3.3f | \n| %3.3f %3.3f %3.3f %3.3f | \n| %3.3f %3.3f %3.3f %3.3f | \n| %3.3f %3.3f %3.3f %3.3f | \n| 3.3f %3.3f %3.3f %3.3f | \n", rotMat[0][0] , rotMat[0][1],  rotMat[0][2], t.pos[0]*1, rotMat[1][0], rotMat[1][1],rotMat[1][2], t.pos[1]*1, rotMat[2][0], rotMat[2][1],rotMat[2][2], t.pos[2]*1, zero, zero, zero, one);

          // proxy->SetHeadPose( rotMat[0][0], rotMat[0][1],rotMat[0][2], t.pos[0]*1,
          //                  rotMat[1][0], rotMat[1][1],rotMat[1][2], t.pos[1]*1,
          //                  rotMat[2][0], rotMat[2][1],rotMat[2][2], t.pos[2]*1,
          //                  0.0, 0.0, 0.0, 1.0 );
          proxy->UpdateVTKObjects();
          //proxy->StillRender();
          }
        }
      }
    }
}

// -------------------------------------------------------------------------fun
vrpn_ANALOGCB AugmentChannelsToRetainLargestMagnitude(const vrpn_ANALOGCB t)
{
  vrpn_ANALOGCB at;
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

// -------------------------------------------------------------------------fun
void VRPN_CALLBACK handleAnalogPos(void *userdata,
                                   const vrpn_ANALOGCB t)
{
  t_user_callback *tData=static_cast<t_user_callback *>(userdata);

  if ( tData->t_counts.size() == 0 )
    {
    tData->t_counts.push_back(0);
    }

  if ( tData->t_counts[0] == 1 )
    {
    tData->t_counts[0] = 0;

    // printf("Rendering\n");
    // printf("%6.3f, %6.3f, %6.3f, %6.3f, %6.3f, %6.3f\n", t.channel[0],
    //        t.channel[1], t.channel[2], t.channel[3], t.channel[4],
    //        t.channel[5]);
    vrpn_ANALOGCB at = AugmentChannelsToRetainLargestMagnitude(t);
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

            viewProxy->GetRenderer()->ResetCameraClippingRange();
            viewProxy->GetRenderWindow()->Render();
          }
      }
    }
  else
    {
      tData->t_counts[0]++;
    }
}
#endif
