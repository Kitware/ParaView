/*=========================================================================

   Program: ParaView
   Module:    ParaViewVRPN.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "ParaViewVRPN.h"

#include <vrpn_Tracker.h>
#include <vrpn_Button.h>
#include <vrpn_Analog.h>
#include <vrpn_Dial.h>
#include <vrpn_Text.h>
#include "vtkMath.h"
#include "pqActiveObjects.h"
#include "pqView.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMDoubleVectorProperty.h"

#include <vtkstd/vector>
#include <iostream>

class t_user_callback
{
public:
  char t_name[vrpn_MAX_TEXT_LEN];
  vtkstd::vector<unsigned> t_counts;
};

class ParaViewVRPN::pqInternals
{
public:
  pqInternals()
    {
      this->Tracker=0;
      this->Button=0;
      this->Analog=0;
      this->Dial=0;
      this->Text=0;
      this->TC1=0;
    }
  ~pqInternals()
    {
      if(this->Tracker!=0)
        {
        delete this->Tracker;
        }
      if(this->Button!=0)
        {
        delete this->Button;
        }
      if(this->Analog!=0)
        {
        delete this->Analog;
        }
      if(this->Dial!=0)
        {
        delete this->Dial;
        }
      if(this->Text!=0)
        {
        delete this->Text;
        }
      if(this->TC1!=0)
        {
        delete this->TC1;
        }
    }

  vrpn_Tracker_Remote *Tracker;
  vrpn_Button_Remote  *Button;
  vrpn_Analog_Remote  *Analog;
  vrpn_Dial_Remote    *Dial;
  vrpn_Text_Receiver  *Text;

  t_user_callback *TC1;
};

void VRPN_CALLBACK handleTrackerPosQuat(void *userdata,
const vrpn_TRACKERCB t)
{
  t_user_callback *tData=static_cast<t_user_callback *>(userdata);

  // Make sure we have a count value for this sensor
  while(tData->t_counts.size() <= static_cast<unsigned>(t.sensor))
    {
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
        double rotMat[3][3];
        vtkMath::QuaternionToMatrix3x3(t.quat,rotMat);
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
          proxy->StillRender();
          }
        }
      }
    }
}

// ----------------------------------------------------------------------------
ParaViewVRPN::ParaViewVRPN()
{
  this->Internals=new pqInternals();
  this->Name=0;
  this->Initialized=false;
}

// ----------------------------------------------------------------------------
void ParaViewVRPN::SetName(const char *name)
{
  if(this->Name!=name)
    {
    int size=strlen(name) + 1;
    if(name && size > 1)
      {
      if(this->Name!=0)
        {
        delete[] this->Name;
        }
      this->Name=new char[size];
      }
    strncpy(this->Name,name,size);
    }
}

// ----------------------------------------------------------------------------
const char *ParaViewVRPN::GetName() const
{
  return this->Name;
}

// ----------------------------------------------------------------------------
bool ParaViewVRPN::GetInitialized() const
{
  return this->Initialized;
}

// ----------------------------------------------------------------------------
void ParaViewVRPN::Init()
{
  this->Internals->Tracker = new vrpn_Tracker_Remote(this->Name);
  this->Internals->Analog = new vrpn_Analog_Remote(this->Name);
  this->Internals->Button = new vrpn_Button_Remote(this->Name);
  this->Internals->Dial = new vrpn_Dial_Remote(this->Name);
  this->Internals->Text = new vrpn_Text_Receiver(this->Name);

  this->Initialized=this->Internals->Tracker!=0
    && this->Internals->Analog!=0
    && this->Internals->Button!=0
    && this->Internals->Dial!=0
    && this->Internals->Text!=0;

  if(this->Initialized)
    {
    this->Internals->TC1=new t_user_callback;
    strncpy(this->Internals->TC1->t_name, this->Name,
            sizeof(this->Internals->TC1->t_name));
    this->Internals->Tracker->register_change_handler(this->Internals->TC1,
                                                      handleTrackerPosQuat);
    }
}

// ----------------------------------------------------------------------------
ParaViewVRPN::~ParaViewVRPN()
{
  delete this->Internals;
  if(this->Name!=0)
    {
    delete[] this->Name;
    }
}

// ----------------------------------------------------------------------------
void ParaViewVRPN::callback()
{
  if(this->Initialized)
    {
//    std::cout << "callback()" << std::endl;
    this->Internals->Tracker->mainloop();
    this->Internals->Button->mainloop();
    this->Internals->Analog->mainloop();
    this->Internals->Dial->mainloop();
    this->Internals->Text->mainloop();
    }
}
