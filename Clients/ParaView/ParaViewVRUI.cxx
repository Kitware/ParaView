/*=========================================================================

   Program: ParaView
   Module:    ParaViewVRUI.cxx

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
#include "ParaViewVRUI.h"

#include "pqActiveObjects.h"
#include "pqView.h"
#include "vruiPipe.h"
#include "vruiServerState.h"
#include "vruiThread.h"
#include "vtkMath.h"
#include "vtkSMCaveRenderViewProxy.h"
#include <QMutex>
#include <QTcpSocket>
#include <QWaitCondition>
#include <iostream>
#include <vector>

class ParaViewVRUI::pqInternals
{
public:
  pqInternals()
  {
    this->Pipe = 0;
    this->State = 0;
    this->Active = false;
    this->Streaming = false;
    this->Thread = 0;
    this->StateMutex = 0;
    this->PacketSignalCond = 0;
    this->PacketSignalCondMutex = 0;
  }
  ~pqInternals()
  {
    if (this->Pipe != 0)
    {
      delete this->Pipe;
    }
    if (this->State != 0)
    {
      delete this->State;
    }
    if (this->Thread != 0)
    {
      delete this->Thread;
    }
  }
  vruiPipe* Pipe;
  vruiServerState* State;
  bool Active;
  bool Streaming;
  vruiThread* Thread;

  QMutex* StateMutex;

  QWaitCondition* PacketSignalCond;
  QMutex* PacketSignalCondMutex;
};

#if 0
void VRUI_CALLBACK handleTrackerPosQuat(void *userdata,
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
  const unsigned tracker_stride = 1;    // Every nth report will be printed

  if ( ++tData->t_counts[t.sensor] >= tracker_stride )
    {
    tData->t_counts[t.sensor] = 0;
    printf("Tracker %s, sensor %d:\n        pos (%5.2f, %5.2f, %5.2f); quat (%5.2f, %5.2f, %5.2f, %5.2f)\n",
           tData->t_name,
           t.sensor,
           t.pos[0], t.pos[1], t.pos[2],
           t.quat[0], t.quat[1], t.quat[2], t.quat[3]);
    }
}
#endif

// ----------------------------------------------------------------------------
ParaViewVRUI::ParaViewVRUI()
{
  this->Internals = new pqInternals();
  this->Name = 0;
  this->Port = 8555;
  this->Initialized = false;
}

// ----------------------------------------------------------------------------
void ParaViewVRUI::SetName(const char* name)
{
  if (this->Name != name)
  {
    int size = strlen(name) + 1;
    if (name && size > 1)
    {
      if (this->Name != 0)
      {
        delete[] this->Name;
      }
      this->Name = new char[size];
    }
    strncpy(this->Name, name, size);
  }
}

// ----------------------------------------------------------------------------
const char* ParaViewVRUI::GetName() const
{
  return this->Name;
}

// ----------------------------------------------------------------------------
int ParaViewVRUI::GetPort() const
{
  return this->Port;
}

// ----------------------------------------------------------------------------
void ParaViewVRUI::SetPort(int port)
{
  this->Port = port;
}

// ----------------------------------------------------------------------------
bool ParaViewVRUI::GetInitialized() const
{
  return this->Initialized;
}

// ----------------------------------------------------------------------------
void ParaViewVRUI::Init()
{
  QTcpSocket* socket = new QTcpSocket;
  socket->connectToHost(QString(this->Name), this->Port); // ReadWrite?
  this->Internals->Pipe = new vruiPipe(socket);
  this->Internals->Pipe->Send(vruiPipe::CONNECT_REQUEST);
  if (!this->Internals->Pipe->WaitForServerReply(30000)) // 30s
  {
    cerr << "Timeout while waiting for CONNECT_REPLY" << endl;
    delete this->Internals->Pipe;
    this->Internals->Pipe = 0;
    return;
  }
  if (this->Internals->Pipe->Receive() != vruiPipe::CONNECT_REPLY)
  {
    cerr << "Mismatching message while waiting for CONNECT_REPLY" << endl;
    delete this->Internals->Pipe;
    this->Internals->Pipe = 0;
    return;
  }

  this->Internals->State = new vruiServerState;
  this->Internals->StateMutex = new QMutex;

  this->Internals->Pipe->ReadLayout(this->Internals->State);

  this->Activate();

  //  this->StartStream();

  this->Initialized = true;
}

// ----------------------------------------------------------------------------
void ParaViewVRUI::Activate()
{
  if (!this->Internals->Active)
  {
    this->Internals->Pipe->Send(vruiPipe::ACTIVATE_REQUEST);
    this->Internals->Active = true;
  }
}

// ----------------------------------------------------------------------------
void ParaViewVRUI::Deactivate()
{
  if (this->Internals->Active)
  {
    this->Internals->Active = false;
    this->Internals->Pipe->Send(vruiPipe::DEACTIVATE_REQUEST);
  }
}

// ----------------------------------------------------------------------------
void ParaViewVRUI::StartStream()
{
  if (this->Internals->Active)
  {
    this->Internals->Thread = new vruiThread;
    this->Internals->Streaming = true;
    this->Internals->Thread->SetPipe(this->Internals->Pipe);
    this->Internals->Thread->SetServerState(this->Internals->State);
    this->Internals->Thread->SetStateMutex(this->Internals->StateMutex);
    this->Internals->Thread->start();

    this->Internals->PacketSignalCond = new QWaitCondition;
    QMutex m;
    m.lock();

    this->Internals->Pipe->Send(vruiPipe::STARTSTREAM_REQUEST);
    this->Internals->PacketSignalCond->wait(&m);
    m.unlock();

    this->Internals->Streaming = true;
  }
}

// ----------------------------------------------------------------------------
void ParaViewVRUI::StopStream()
{
  if (this->Internals->Streaming)
  {
    this->Internals->Streaming = false;
    this->Internals->Pipe->Send(vruiPipe::STOPSTREAM_REQUEST);
    this->Internals->Thread->wait();
  }
}

// ----------------------------------------------------------------------------
ParaViewVRUI::~ParaViewVRUI()
{
  this->StopStream();

  this->Deactivate();

  delete this->Internals;
  if (this->Name != 0)
  {
    delete[] this->Name;
  }
}

// ----------------------------------------------------------------------------
void ParaViewVRUI::callback()
{
  if (this->Initialized)
  {
    // std::cout << "callback()" << std::endl;

    this->Internals->StateMutex->lock();

    // Print position and orientation of tracker 0. (real callback)
    this->PrintPositionOrientation();

    this->Internals->StateMutex->unlock();
    this->GetNextPacket(); // for the next step
  }
}

// ----------------------------------------------------------------------------
void ParaViewVRUI::GetNextPacket()
{
  if (this->Internals->Active)
  {
    if (this->Internals->Streaming)
    {
      // With a thread
      this->Internals->PacketSignalCondMutex->lock();
      this->Internals->PacketSignalCond->wait(this->Internals->PacketSignalCondMutex);
      this->Internals->PacketSignalCondMutex->unlock();
    }
    else
    {
      // With a loop
      this->Internals->Pipe->Send(vruiPipe::PACKET_REQUEST);
      if (this->Internals->Pipe->WaitForServerReply(10000))
      {
        if (this->Internals->Pipe->Receive() != vruiPipe::PACKET_REPLY)
        {
          cout << "PVRUI Mismatching message while waiting for PACKET_REPLY" << endl;
        }
        else
        {
          this->Internals->StateMutex->lock();
          this->Internals->Pipe->ReadState(this->Internals->State);
          this->Internals->StateMutex->unlock();

          //          this->PacketNotificationMutex->lock();
          //          this->PacketNotificationMutex->unlock();
        }
      }
      else
      {
        cout << "timeout for PACKET_REPLY" << endl;
      }
    }
  }
}

// ----------------------------------------------------------------------------
void ParaViewVRUI::PrintPositionOrientation()
{
  std::vector<vtkSmartPointer<vruiTrackerState> >* trackers =
    this->Internals->State->GetTrackerStates();

  float pos[3];
  float q[4];
  (*trackers)[0]->GetPosition(pos);
  (*trackers)[0]->GetUnitQuaternion(q);

  // cout << "pos=("<< pos[0] << "," << pos[1] << "," << pos[2] << ")" << endl;
  // cout << "q=("<< q[0] << "," << q[1] << "," << q[2] << "," << q[3] << ")"
  //      << endl;

  std::vector<bool>* buttons = this->Internals->State->GetButtonStates();
  // cout << "button0=" << (*buttons)[0] << endl;
  pqView* view = 0;
  view = pqActiveObjects::instance().activeView();
  if (view)
  {
    vtkSMCaveRenderViewProxy* proxy = 0;
    proxy = vtkSMCaveRenderViewProxy::SafeDownCast(view->getViewProxy());
    if (proxy)
    {
      double rotMat[3][3];
      vtkMath::QuaternionToMatrix3x3((double*)q, rotMat);
      proxy->SetHeadPose(rotMat[0][0], rotMat[0][1], rotMat[0][2], pos[0] * 1, rotMat[1][0],
        rotMat[1][1], rotMat[1][2], pos[1] * 1, rotMat[2][0], rotMat[2][1], rotMat[2][2],
        pos[2] * 1, 0.0, 0.0, 0.0, 1.0);
    }
  }
}
