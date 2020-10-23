/*=========================================================================

   Program: ParaView
   Module:    vtkVRUIPipe.cxx

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
#include "vtkVRUIPipe.h"

#include <cassert>

#include <QDebug>
#include <QTcpSocket>

#include "vtkVRUIServerState.h"

typedef unsigned short MessageTagPropocol;

// ----------------------------------------------------------------------------
vtkVRUIPipe::vtkVRUIPipe(QTcpSocket* socket)
{
  assert("pre: socket_exist" && socket != 0);
  this->Socket = socket;
}

// ----------------------------------------------------------------------------
vtkVRUIPipe::~vtkVRUIPipe()
{
}

// ----------------------------------------------------------------------------
void vtkVRUIPipe::Send(MessageTag m)
{
  MessageTagPropocol message = m;
  // std::cout << "Sending : " << this->GetString( m ) << std::endl;
  this->Socket->write(reinterpret_cast<const char*>(&message), sizeof(MessageTagPropocol));
  this->Socket->flush();
}

// ----------------------------------------------------------------------------
bool vtkVRUIPipe::WaitForServerReply(int vtkNotUsed(msecs))
{
  // std::cout<< "in" <<std::endl;
  bool status = this->Socket->waitForReadyRead(500);
  // std::cout<< "out : " << status <<std::endl;
  return status;
}

// ----------------------------------------------------------------------------
vtkVRUIPipe::MessageTag vtkVRUIPipe::Receive()
{
  MessageTagPropocol message;

  qint64 bytes = 0;

  while (bytes != sizeof(MessageTagPropocol)) // 2
  {
    // std::cout<< "Waiting to receive" <<std::endl;
    bytes = this->Socket->read(reinterpret_cast<char*>(&message), sizeof(MessageTagPropocol));
#ifdef VRUI_ENABLE_DEBUG
    if (bytes)
      cout << "bytes=" << bytes << endl;

    cout << "sizeof=" << sizeof(MessageTagPropocol) << endl;
    std::cout << "Received : " << this->GetString(static_cast<MessageTag>(message))
              << static_cast<MessageTag>(message) << std::endl;
#endif
  }
  return static_cast<MessageTag>(message);
}

// ----------------------------------------------------------------------------
void vtkVRUIPipe::ReadLayout(vtkVRUIServerState* state)
{
  assert("pre: state_exists" && state != 0);

  int value;
  this->Socket->read(reinterpret_cast<char*>(&value), sizeof(int));
  state->GetTrackerStates()->resize(value);
  int i = 0;
  while (i < value)
  {
    (*(state->GetTrackerStates()))[i] = vtkVRUITrackerState::New();
    ++i;
  }

  cout << "number of trackers: " << value << endl;

  this->Socket->read(reinterpret_cast<char*>(&value), sizeof(int));
  state->GetButtonStates()->resize(value);

  cout << "number of buttons: " << value << endl;

  this->Socket->read(reinterpret_cast<char*>(&value), sizeof(int));

  state->GetValuatorStates()->resize(value);

  cout << "number of valuators: " << value << endl;
}

// ----------------------------------------------------------------------------
void vtkVRUIPipe::ReadState(vtkVRUIServerState* state)
{
  assert("pre: state_exists" && state != 0);

  // read all trackers states.
  std::vector<vtkSmartPointer<vtkVRUITrackerState> >* trackers = state->GetTrackerStates();
  size_t i = 0;
  size_t c = trackers->size();

  quint64 readSize;

  while (i < c)
  {
    vtkVRUITrackerState* tracker = (*trackers)[i].GetPointer();
    readSize =
      this->Socket->read(reinterpret_cast<char*>(tracker->GetPosition()), 3 * sizeof(float));
    if (readSize < 3 * sizeof(float))
    {
      qDebug() << "position: " << readSize;
    }
    readSize =
      this->Socket->read(reinterpret_cast<char*>(tracker->GetUnitQuaternion()), 4 * sizeof(float));
    if (readSize < 4 * sizeof(float))
    {
      qDebug() << "quat:" << readSize;
    }
    readSize =
      this->Socket->read(reinterpret_cast<char*>(tracker->GetLinearVelocity()), 3 * sizeof(float));
    if (readSize < 3 * sizeof(float))
    {
      qDebug() << "lv: " << readSize;
    }
    readSize =
      this->Socket->read(reinterpret_cast<char*>(tracker->GetAngularVelocity()), 3 * sizeof(float));
    if (readSize < 3 * sizeof(float))
    {
      qDebug() << "av: " << readSize;
    }
    ++i;
  }
  // read all buttons states.
  std::vector<bool>* buttons = state->GetButtonStates();
  i = 0;
  c = buttons->size();
  while (i < c)
  {
    bool value;
    readSize = this->Socket->read(reinterpret_cast<char*>(&value), sizeof(bool));
    if (readSize < sizeof(bool))
    {
      qDebug() << "button : " << i << readSize;
    }
    (*buttons)[i] = value;
    ++i;
  }

  // read all valuators states.
  std::vector<float>* valuators = state->GetValuatorStates();
  i = 0;
  c = valuators->size();
  while (i < c)
  {
    float value;
    readSize = this->Socket->read(reinterpret_cast<char*>(&value), sizeof(float));
    if (readSize < sizeof(float))
    {
      qDebug() << "analog : " << i << readSize;
    }
    (*valuators)[i] = value;
    ++i;
  }
}
