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
#include <pqDataRepresentation.h>
#include "vtkSMRenderViewProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include "vtkVRPNCallBackHandlers.h"
#include <QDateTime>
#include <vtkstd/vector>
#include <iostream>


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
    }

  vrpn_Tracker_Remote *Tracker;
  vrpn_Button_Remote  *Button;
  vrpn_Analog_Remote  *Analog;
  vrpn_Dial_Remote    *Dial;
  vrpn_Text_Receiver  *Text;
};


// ----------------------------------------------------------------------------
ParaViewVRPN::ParaViewVRPN()
{
  this->Internals=new pqInternals();
  this->Initialized=false;
  this->_Stop = false;
}

// ----------------------------------------------------------------------------
void ParaViewVRPN::SetName(std::string name)
{
  this->Name = name;
}

// ----------------------------------------------------------------------------
bool ParaViewVRPN::GetInitialized() const
{
  return this->Initialized;
}

// ----------------------------------------------------------------------------
void ParaViewVRPN::Init()
{
  this->Internals->Tracker = new vrpn_Tracker_Remote(this->Name.c_str());
  this->Internals->Analog = new vrpn_Analog_Remote(this->Name.c_str());
  this->Internals->Button = new vrpn_Button_Remote(this->Name.c_str());
  this->Internals->Dial = new vrpn_Dial_Remote(this->Name.c_str());
  this->Internals->Text = new vrpn_Text_Receiver(this->Name.c_str());

  this->Initialized= ( this->Internals->Tracker!=0
                       && this->Internals->Analog!=0
                       && this->Internals->Button!=0
                       && this->Internals->Dial!=0
                       && this->Internals->Text!=0 );

  if(this->Initialized)
    {
    this->Internals->Tracker->register_change_handler(static_cast<void*>( this ),
                                                      handleTrackerChange );
    this->Internals->Analog->register_change_handler(static_cast<void*>( this ),
                                                     handleAnalogChange );
    this->Internals->Button->register_change_handler( static_cast<void*>( this ),
                                                      handleButtonChange );
    }
}

// ----------------------------------------------------------------------------
ParaViewVRPN::~ParaViewVRPN()
{
  delete this->Internals;
}

// ----------------------------------------------------------------------------
void ParaViewVRPN::run()
{
  while ( !this->_Stop )
    {
    if(this->Initialized)
      {
      //    std::cout << "callback()" << std::endl;
      this->Internals->Tracker->mainloop();
      this->Internals->Button->mainloop();
      this->Internals->Analog->mainloop();
      this->Internals->Dial->mainloop();
      this->Internals->Text->mainloop();
      //msleep( 40 );
      }
    }
}

void ParaViewVRPN::terminate()
{
  this->_Stop = true;
  vtkThread::terminate();
}

void ParaViewVRPN::SetQueue( vtkVRQueue* queue )
{
  this->EventQueue = queue;
}


void ParaViewVRPN::NewAnalogValue(vrpn_ANALOGCB data)
{
  vtkVREventData temp;
  temp.connId = this->Name;
  temp.eventType = ANALOG_EVENT;
  temp.timeStamp =   QDateTime::currentDateTime().toTime_t();
  temp.data.analog.num_channel = data.num_channel;
  for ( int i=0 ; i<data.num_channel ;++i )
    {
    temp.data.analog.channel[i] = data.channel[i];
    }
  this->EventQueue->enqueue( temp );
}

void ParaViewVRPN::NewButtonValue(vrpn_BUTTONCB data)
{
  vtkVREventData temp;
  temp.connId = this->Name;
  temp.eventType = BUTTON_EVENT;
  temp.timeStamp =   QDateTime::currentDateTime().toTime_t();
  temp.data.button.button = data.button;
  temp.data.button.state = data.state;
  this->EventQueue->enqueue( temp );
}

void ParaViewVRPN::NewTrackerValue(vrpn_TRACKERCB data )
{
  vtkVREventData temp;
  temp.connId = this->Name;
  temp.eventType = TRACKER_EVENT;
  temp.timeStamp =   QDateTime::currentDateTime().toTime_t();
  temp.data.tracker.sensor = data.sensor;
  temp.data.tracker.pos[0] = data.pos[0];
  temp.data.tracker.pos[1] = data.pos[1];
  temp.data.tracker.pos[2] = data.pos[2];
  temp.data.tracker.quat[0] = data.quat[0];
  temp.data.tracker.quat[1] = data.quat[1];
  temp.data.tracker.quat[2] = data.quat[2];
  temp.data.tracker.quat[3] = data.quat[3];
  this->EventQueue->enqueue( temp );
}
