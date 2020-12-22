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

#include "vtkVRUIServerState.h"

#ifdef QTSOCK
#include <QTcpSocket>
#else
#include <QDebug>
#endif

#include <cassert>
#include <unistd.h>

// typedef unsigned short MessageTagProtocol;
typedef uint16_t MessageTagProtocol;

// ----------------------------------------------------------------------------
// Constructor method
#ifdef QTSOCK
vtkVRUIPipe::vtkVRUIPipe(QTcpSocket* socket)
{
  assert("pre: socket_exist" && socket != 0);
  this->Socket = socket;
  this->protocol = 0; /* NOTE: this is where the VRUI protocol is specified */
}
#else
vtkVRUIPipe::vtkVRUIPipe(int socket)
{
  this->Socket = socket;
  this->protocol = 0; /* NOTE: this is where the VRUI protocol is specified */
}
#endif

// ----------------------------------------------------------------------------
// Destructor method
vtkVRUIPipe::~vtkVRUIPipe()
{
}

// ----------------------------------------------------------------------------
void vtkVRUIPipe::Send(MessageTag m)
{
  MessageTagProtocol message = m;
// std::cout << "Sending : " << this->GetString(m) << std::endl;
#ifdef QTSOCK
  this->Socket->write(reinterpret_cast<const char*>(&message), sizeof(message));
  this->Socket->flush();
#else
  write(this->Socket, reinterpret_cast<const char*>(&message), sizeof(message));
#endif
}

// ----------------------------------------------------------------------------
void vtkVRUIPipe::Send(uint32_t value)
{
  uint32_t message = value;
// std::cout << "Sending : " << this->GetString( m ) << std::endl;
#ifdef QTSOCK
  this->Socket->write(reinterpret_cast<const char*>(&message), sizeof(message));
  this->Socket->flush();
#else
  write(this->Socket, reinterpret_cast<const char*>(&message), sizeof(message));
#endif
}

// ----------------------------------------------------------------------------
bool vtkVRUIPipe::WaitForServerReply(int vtkNotUsed(msecs))
{
#ifdef QTSOCK
  // std::cout<< "in" <<std::endl;
  bool status = this->Socket->waitForReadyRead(500);
  // std::cout<< "out : " << status <<std::endl;
  return status;
#else
  static bool warned = false;
  if (!warned)
  {
    qWarning("vtkVRUIPipe::WaitForServerReply() is not fully implemented -- may be a problem if "
             "Vrui Daemon is not running!");
    warned = true;
  }
  return true;
#endif
}

// ----------------------------------------------------------------------------
vtkVRUIPipe::MessageTag vtkVRUIPipe::Receive()
{
  MessageTagProtocol message = UNKNOWN_MESSAGE;

  qint64 bytes = 0;

  while (bytes != sizeof(MessageTagProtocol)) // s/b 2 bytes
  {
    std::cout << "Waiting to receive\n";
#ifdef QTSOCK
    bytes = this->Socket->read(reinterpret_cast<char*>(&message), sizeof(MessageTagProtocol));
#else
    // WRS-TODO: the following line seems to block when starting after running and then stopping --
    // need to investigate
    bytes = read(this->Socket, reinterpret_cast<char*>(&message), sizeof(MessageTagProtocol));
    if (bytes < 0)
    {
      qDebug() << "vtkVRUIPipe: Socket read error -- no message packet from Vrui Server";
      sleep(2);
    }
    std::cout << ">> received\n";
#endif
#if defined(VRUI_ENABLE_DEBUG) || 0
    if (bytes != 0)
      std::cout << "bytes = " << bytes << endl;

    std::cout << "sizeof = " << sizeof(MessageTagProtocol) << endl;
    std::cout << "Recieved: " << this->GetString(static_cast<MessageTag>(message)) << " ("
              << static_cast<MessageTag>(message) << ")" << std::endl;
#endif
  }
  return static_cast<MessageTag>(message);
}

// ----------------------------------------------------------------------------
void vtkVRUIPipe::ReadLayout(vtkVRUIServerState* state)
{
  assert("pre: state_exists" && state != 0);

  uint32_t value; /* Generic 4-byte "value" returned by VRUI VRDeviceDaemon */
  ssize_t bytes;  /* The number of bytes returned from a socket read */

  cout << "Vrui Server: using protocol " << this->protocol << endl;
  /* Read the protocol value */
  if (this->protocol > 0)
  {
    cout << "Reading Vrui Protocol value" << endl;
    bytes = read(this->Socket, reinterpret_cast<char*>(&value), sizeof(value));
    if (bytes < 0)
    {
      qDebug() << "Socket readlayout protocol number error";
    }
    cout << "Vrui Server: reporting protocol " << value << endl;
  }

/* Read the number of 6-DOF position trackers */
#ifdef QTSOCK
  this->Socket->read(reinterpret_cast<char*>(&value), sizeof(value));
#else
  bytes = read(this->Socket, reinterpret_cast<char*>(&value), sizeof(value));
  if (bytes < 0)
  {
    qDebug() << "Socket readlayout tracker error";
  }
#endif
  state->GetTrackerStates()->resize(value);
  for (int count = 0; count < value; count++)
  {
    (*(state->GetTrackerStates()))[count] = vtkVRUITrackerState::New();
  }

  cout << "Vrui Server: reporting " << value << " trackers" << endl;

/* Read the number of buttons */
#ifdef QTSOCK
  this->Socket->read(reinterpret_cast<char*>(&value), sizeof(value));
#else
  bytes = read(this->Socket, reinterpret_cast<char*>(&value), sizeof(value));
  if (bytes < 0)
  {
    qDebug() << "Socket readlayout buttons error";
  }
#endif
  state->GetButtonStates()->resize(value);

  cout << "Vrui Server: reporting " << value << " buttons" << endl;

/* Read the number of valuators */
#ifdef QTSOCK
  this->Socket->read(reinterpret_cast<char*>(&value), sizeof(value));
#else
  bytes = read(this->Socket, reinterpret_cast<char*>(&value), sizeof(value));
  if (bytes < 0)
  {
    qDebug() << "Socket readlayout valuators error";
  }
#endif
  state->GetValuatorStates()->resize(value);

  cout << "Vrui Server: reporting " << value << " valuators " << endl;
}

// ----------------------------------------------------------------------------
// WARNING: this assumes that floats are 32bits (4 bytes) -- also assumes little endian
void vtkVRUIPipe::ReadState(vtkVRUIServerState* state)
{
  assert("pre: state_exists" && state != 0);

  // read all trackers states.
  std::vector<vtkSmartPointer<vtkVRUITrackerState> >* trackers = state->GetTrackerStates();
  quint64 readSize;
  int count;
  int num_inputs;

  num_inputs = trackers->size();
  for (count = 0; count < num_inputs; count++)
  {
    vtkVRUITrackerState* tracker = (*trackers)[count].GetPointer();
#ifdef QTSOCK
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
#else
    readSize =
      read(this->Socket, reinterpret_cast<char*>(tracker->GetPosition()), 3 * sizeof(float));
    if (readSize < 3 * sizeof(float))
    {
      qDebug() << "position: " << readSize;
    }
    readSize =
      read(this->Socket, reinterpret_cast<char*>(tracker->GetUnitQuaternion()), 4 * sizeof(float));
    if (readSize < 4 * sizeof(float))
    {
      qDebug() << "quat:" << readSize;
    }
    readSize =
      read(this->Socket, reinterpret_cast<char*>(tracker->GetLinearVelocity()), 3 * sizeof(float));
    if (readSize < 3 * sizeof(float))
    {
      qDebug() << "lv: " << readSize;
    }
    readSize =
      read(this->Socket, reinterpret_cast<char*>(tracker->GetAngularVelocity()), 3 * sizeof(float));
    if (readSize < 3 * sizeof(float))
    {
      qDebug() << "av: " << readSize;
    }
#endif
  }
  // read all buttons states.
  std::vector<bool>* buttons = state->GetButtonStates();
  num_inputs = buttons->size();
  for (count = 0; count < num_inputs; count++)
  {
    bool value;
#ifdef QTSOCK
    readSize = this->Socket->read(reinterpret_cast<char*>(&value), sizeof(value));
#else
    readSize = read(this->Socket, reinterpret_cast<char*>(&value), sizeof(value));
#endif
    if (readSize < sizeof(value))
      qDebug() << "button : " << count << readSize;
    (*buttons)[count] = value;
  }

  // read all valuators states.
  std::vector<float>* valuators = state->GetValuatorStates();
  num_inputs = valuators->size();
  for (count = 0; count < num_inputs; count++)
  {
    float value;
#ifdef QTSOCK
    readSize = this->Socket->read(reinterpret_cast<char*>(&value), sizeof(value));
#else
    readSize = read(this->Socket, reinterpret_cast<char*>(&value), sizeof(value));
#endif
    if (readSize < sizeof(value))
      qDebug() << "analog : " << count << readSize;
    (*valuators)[count] = value;
  }
}
