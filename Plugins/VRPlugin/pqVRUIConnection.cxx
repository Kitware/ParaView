/*=========================================================================

  Program: ParaView
  Module:    vtkVRUIConnection.cxx

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
#include "pqVRUIConnection.h"

#include "pqActiveObjects.h"
#include "pqView.h"
#include "vtkMath.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkTransform.h"
#include "vtkVRUIPipe.h"
#include "vtkVRUIServerState.h"
#include "vtkVRUITrackerState.h"

#include <QDateTime>
#include <QDebug>
#include <QMutex>
#include <QTcpSocket>

#include <algorithm>
#include <iostream>
#include <pqDataRepresentation.h>
#include <sstream>
#include <vector>
#include <vtkCamera.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>

// ----------------------------------------------------------------------------
class pqVRUIConnection::pqInternals
{
public:
  // --------------------------------------------------------------------------
  pqInternals()
  {
    this->Socket = nullptr;
    this->Active = false;
    this->Pipe = 0;
    this->State = 0;
    this->StateMutex = 0;
    this->Streaming = false; // streaming
    //  this->PacketSignalCond=0;        // for streaming
    //   this->PacketSignalCondMutex=0; // for streaming
  }

  // --------------------------------------------------------------------------
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
  }

  QTcpSocket* Socket;
  bool Active;
  vtkVRUIPipe* Pipe;
  vtkVRUIServerState* State;
  QMutex* StateMutex;
  bool Streaming; // streaming
  // QWaitCondition *PacketSignalCond; // for streaming
  // QMutex *PacketSignalCondMutex;    // for streaming

  // --------------------------------------------------------------------------
  void initSocket(std::string address, std::string port)
  {
    this->Socket = new QTcpSocket();
#ifdef VRUI_ENABLE_DEBUG
    qDebug() << QString(address.c_str()) << "::" << QString(port.c_str()).toInt();
#endif

    this->Socket->connectToHost(QString(address.c_str()),
      QString(port.c_str()).toInt()); // ReadWrite?
  }

  // --------------------------------------------------------------------------
  void initPipe() { this->Pipe = new vtkVRUIPipe(this->Socket); }

  // --------------------------------------------------------------------------
  bool connect()
  {
#ifdef VRUI_ENABLE_DEBUG
    std::cout << "Trying to connect" << std::endl;
#endif
    this->Pipe->Send(vtkVRUIPipe::CONNECT_REQUEST);
    if (!this->Pipe->WaitForServerReply(30000)) // 30s
    {
      cerr << "Timeout while waiting for CONNECT_REPLY" << endl;
      delete this->Pipe;
      this->Pipe = 0;
      return false;
    }
    if (this->Pipe->Receive() != vtkVRUIPipe::CONNECT_REPLY)
    {
      cerr << "Mismatching message while waiting for CONNECT_REPLY" << endl;
      delete this->Pipe;
      this->Pipe = 0;
      return false;
    }
    this->State = new vtkVRUIServerState;
    this->StateMutex = new QMutex;
    this->Pipe->ReadLayout(this->State);
    return true;
  }

  // --------------------------------------------------------------------------
  void activate()
  {
    if (!this->Active)
    {
      this->Pipe->Send(vtkVRUIPipe::ACTIVATE_REQUEST);
      this->Active = true;
    }
  }

  // --------------------------------------------------------------------------
  void deactivate()
  {
    if (this->Active)
    {
      this->Active = false;
      this->Pipe->Send(vtkVRUIPipe::DEACTIVATE_REQUEST);
    }
  }

  // --------------------------------------------------------------------------
  void startStream()
  {
    if (this->Active)
    {
#ifdef VRUI_ENABLE_DEBUG
      std::cout << "start streaming" << std::endl;
#endif
      this->Streaming = true;
      this->Pipe->Send(vtkVRUIPipe::STARTSTREAM_REQUEST);
    }
  }

  // --------------------------------------------------------------------------
  void stopStream()
  {
    if (this->Streaming)
    {
      this->Streaming = false;
      this->Pipe->Send(vtkVRUIPipe::STOPSTREAM_REQUEST);
    }
  }

  // --------------------------------------------------------------------------
  // Streaming routine called when streaming is used
  void readStream()
  {
    this->StateMutex->lock();
    this->Pipe->ReadState(this->State);
    this->StateMutex->unlock();
    bool done = false;
    vtkVRUIPipe::MessageTag m = this->Pipe->Receive();
    switch (m)
    {
      case vtkVRUIPipe::PACKET_REPLY:
#ifdef VRUI_ENABLE_DEBUG
        cout << "thread:PACKET_REPLY ok : tag=" << m << endl;
#endif
        this->StateMutex->lock();
        this->Pipe->ReadState(this->State);
        this->StateMutex->unlock();

        break;
      case vtkVRUIPipe::STOPSTREAM_REPLY:
        cout << "thread:STOPSTREAM_REPLY ok : tag=" << m << endl;
        done = true;
        break;
      default:
        cerr << "thread: Mismatching message while waiting for PACKET_REPLY: tag="
             << this->Pipe->GetString(m) << "::" << m << endl;
        done = true;
        break;
    }
  }
};

// ----------------------------------------------------------------------------
pqVRUIConnection::pqVRUIConnection(QObject* parentObject)
  : Superclass(parentObject)
{
  this->Internals = new pqInternals();
  this->Initialized = false;
  this->_Stop = false;
  this->Address = "";
  this->Port = "8555";
  this->Name = "";
  this->Type = "VRUI";
  this->TrackerPresent = false;
  this->AnalogPresent = false;
  this->ButtonPresent = false;
  this->TrackerTransformPresent = false;
  this->Transformation = vtkMatrix4x4::New();
  this->Matrix = vtkMatrix4x4::New();
  this->ZUpToYUpMatrix = vtkMatrix4x4::New();
}

// ----------------------------------------------------------------------------
pqVRUIConnection::~pqVRUIConnection()
{
  this->ZUpToYUpMatrix->Delete();
  this->Matrix->Delete();
  this->Transformation->Delete();
  this->Internals->stopStream();
  this->Internals->deactivate();
  delete this->Internals;
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::addButton(std::string id, std::string name)
{
  std::stringstream returnStr;
  returnStr << "button." << id;
  this->ButtonMapping[returnStr.str()] = name;
  this->ButtonPresent = true;
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::addAnalog(std::string id, std::string name)
{
  std::stringstream returnStr;
  returnStr << "analog." << id;
  this->AnalogMapping[returnStr.str()] = name;
  this->AnalogPresent = true;
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::addTracking(std::string id, std::string name)
{
  std::stringstream returnStr;
  returnStr << "tracker." << id;
  this->TrackerMapping[returnStr.str()] = name;
  this->TrackerPresent = true;
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::setName(std::string name)
{
  this->Name = name;
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::setAddress(std::string address)
{
  this->Address = address;
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::setQueue(vtkVRQueue* queue)
{
  this->EventQueue = queue;
}

// ----------------------------------------------------------------------------
bool pqVRUIConnection::init()
{
  // Initialize the socket connection;
  this->Internals->initSocket(this->Address, this->Port);
  this->Internals->initPipe();
  if (!this->Internals->connect())
    return false;
  this->Internals->activate();
  // this->Internals->startStream();
  this->Initialized = true;
  return true;
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::run()
{
  while (!this->_Stop)
  {
    if (this->Initialized)
    {
      if (this->Internals->Streaming)
      {
        this->Internals->readStream();
      }
      else
      {
        this->callback();
      }
    }
  }
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::stop()
{
  this->_Stop = true;
  this->Internals->stopStream();
  this->Internals->deactivate();
  this->Initialized = false;
  QThread::wait();
}

// ----------------------------------------------------------------------------
std::string pqVRUIConnection::name(int eventType, int id)
{
  std::stringstream returnStr, connection, e;
  if (this->Name.size())
    returnStr << this->Name << ".";
  else
    returnStr << this->Address << ".";
  switch (eventType)
  {
    case ANALOG_EVENT:
      e << "analog." << id;
      if (this->AnalogMapping.find(e.str()) != this->AnalogMapping.end())
        returnStr << this->AnalogMapping[e.str()];
      else
        returnStr << e.str();
      break;
    case BUTTON_EVENT:
      e << "button." << id;
      if (this->ButtonMapping.find(e.str()) != this->ButtonMapping.end())
        returnStr << this->ButtonMapping[e.str()];
      else
        returnStr << e.str();
      break;
    case TRACKER_EVENT:
      e << "tracker." << id;
      if (this->TrackerMapping.find(e.str()) != this->TrackerMapping.end())
        returnStr << this->TrackerMapping[e.str()];
      else
        returnStr << e.str();
      break;
  }
  return returnStr.str();
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::verifyConfig(const char* id, const char* name)
{
  if (!id)
  {
    qWarning() << "\"id\" should be specified";
  }
  if (!name)
  {
    qWarning() << "\"name\" should be specified";
  }
}

// ----------------------------------------------------------------------------
bool pqVRUIConnection::configure(vtkPVXMLElement* child, vtkSMProxyLocator*)
{
  bool returnVal = false;
  if (child->GetName() && strcmp(child->GetName(), "VRUIConnection") == 0)
  {
    for (unsigned cc = 0; cc < child->GetNumberOfNestedElements(); ++cc)
    {
      vtkPVXMLElement* e = child->GetNestedElement(cc);
      if (e && e->GetName())
      {
        const char* id = e->GetAttributeOrEmpty("id");
        const char* name = e->GetAttributeOrEmpty("name");
        this->verifyConfig(id, name);

        if (strcmp(e->GetName(), "Button") == 0)
        {
          this->addButton(id, name);
        }
        else if (strcmp(e->GetName(), "Analog") == 0)
        {
          this->addAnalog(id, name);
        }
        else if (strcmp(e->GetName(), "Tracker") == 0)
        {
          this->addTracking(id, name);
        }
        else if (strcmp(e->GetName(), "TrackerTransform") == 0)
        {
          this->configureTransform(e);
        }

        else
        {
          qWarning() << "Unknown Device type: \"" << e->GetName() << "\"";
        }
        returnVal = true;
      }
    }
  }
  return returnVal;
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::configureTransform(vtkPVXMLElement* child)
{
  if (child->GetName() && strcmp(child->GetName(), "TrackerTransform") == 0)
  {
    child->GetVectorAttribute("value", 16, (double*)this->Transformation->Element);
    this->TrackerTransformPresent = true;
  }
}

// ----------------------------------------------------------------------------
vtkPVXMLElement* pqVRUIConnection::saveConfiguration() const
{
  vtkPVXMLElement* child = vtkPVXMLElement::New();
  child->SetName("VRUIConnection");
  child->AddAttribute("name", this->Name.c_str());
  child->AddAttribute("address", this->Address.c_str());
  child->AddAttribute("port", this->Port.c_str());
  saveButtonEventConfig(child);
  saveAnalogEventConfig(child);
  saveTrackerEventConfig(child);
  saveTrackerTransformationConfig(child);
  return child;
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::saveButtonEventConfig(vtkPVXMLElement* child) const
{
  if (!this->ButtonPresent)
    return;
  for (std::map<std::string, std::string>::const_iterator it = this->ButtonMapping.begin();
       it != this->ButtonMapping.end(); ++it)
  {
    std::string key = it->first;
    std::string value = it->second;
    std::replace(key.begin(), key.end(), '.', ' ');
    std::istringstream stm(key);
    std::vector<std::string> token;
    for (;;)
    {
      std::string word;
      if (!(stm >> word))
        break;
      token.push_back(word);
    }
    vtkPVXMLElement* e = vtkPVXMLElement::New();
    if (strcmp(token[0].c_str(), "button") == 0)
    {
      e->SetName("Button");
      e->AddAttribute("id", token[1].c_str());
      e->AddAttribute("name", value.c_str());
    }
    child->AddNestedElement(e);
    e->FastDelete();
  }
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::saveAnalogEventConfig(vtkPVXMLElement* child) const
{
  if (!this->AnalogPresent)
    return;
  for (std::map<std::string, std::string>::const_iterator it = this->AnalogMapping.begin();
       it != this->AnalogMapping.end(); ++it)
  {
    std::string key = it->first;
    std::string value = it->second;
    std::replace(key.begin(), key.end(), '.', ' ');
    std::istringstream stm(key);
    std::vector<std::string> token;
    for (;;)
    {
      std::string word;
      if (!(stm >> word))
        break;
      token.push_back(word);
    }
    vtkPVXMLElement* e = vtkPVXMLElement::New();
    if (strcmp(token[0].c_str(), "analog") == 0)
    {
      e->SetName("Analog");
      e->AddAttribute("id", token[1].c_str());
      e->AddAttribute("name", value.c_str());
    }
    child->AddNestedElement(e);
    e->FastDelete();
  }
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::saveTrackerEventConfig(vtkPVXMLElement* child) const
{
  if (!this->TrackerPresent)
    return;
  for (std::map<std::string, std::string>::const_iterator it = this->TrackerMapping.begin();
       it != this->TrackerMapping.end(); ++it)
  {
    std::string key = it->first;
    std::string value = it->second;
    std::replace(key.begin(), key.end(), '.', ' ');
    std::istringstream stm(key);
    std::vector<std::string> token;
    for (;;)
    {
      std::string word;
      if (!(stm >> word))
        break;
      token.push_back(word);
    }
    vtkPVXMLElement* e = vtkPVXMLElement::New();
    if (strcmp(token[0].c_str(), "tracker") == 0)
    {
      e->SetName("Tracker");
      e->AddAttribute("id", token[1].c_str());
      e->AddAttribute("name", value.c_str());
    }
    child->AddNestedElement(e);
    e->FastDelete();
  }
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::saveTrackerTransformationConfig(vtkPVXMLElement* child) const
{
  if (!this->TrackerTransformPresent)
    return;
  vtkPVXMLElement* transformationMatrix = vtkPVXMLElement::New();
  transformationMatrix->SetName("TrackerTransform");
  std::stringstream matrix;
  for (int i = 0; i < 16; ++i)
  {
    matrix << double(*((double*)this->Transformation->Element + i)) << " ";
  }
  transformationMatrix->AddAttribute("value", matrix.str().c_str());
  child->AddNestedElement(transformationMatrix);
  transformationMatrix->FastDelete();
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::setTransformation(vtkMatrix4x4* matrix)
{
  for (int i = 0; i < 4; ++i)
  {
    for (int j = 0; j < 4; ++j)
    {
      this->Transformation->SetElement(i, j, matrix->GetElement(i, j));
    }
  }
  this->TrackerTransformPresent = true;
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::callback()
{
  if (this->Initialized)
  {

#ifdef VRUI_ENABLE_DEBUG
    std::cout << "callback()" << std::endl;
#endif
    this->Internals->StateMutex->lock();
    this->getAndEnqueueButtonData();
    this->getAndEnqueueAnalogData();
    this->getAndEnqueueTrackerData();
    this->Internals->StateMutex->unlock();

    this->getNextPacket(); // for the next step
  }
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::getNextPacket()
{
  if (this->Internals->Active)
  {
    if (this->Internals->Streaming)
    {
      // With a thread
      // this->Internals->PacketSignalCondMutex->lock();
      // this->Internals->PacketSignalCond->wait(this->Internals->PacketSignalCondMutex);
      // this->Internals->PacketSignalCondMutex->unlock();
    }
    else
    {
      // With a loop
      this->Internals->Pipe->Send(vtkVRUIPipe::PACKET_REQUEST);
      if (this->Internals->Pipe->WaitForServerReply(30000))
      {
        if (this->Internals->Pipe->Receive() != vtkVRUIPipe::PACKET_REPLY)
        {
          cout << "VRUI Mismatching message while waiting for PACKET_REPLY" << std::endl;
          abort();
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
void pqVRUIConnection::newAnalogValue(std::vector<float>* data)
{
  vtkVREventData temp;
  temp.connId = this->Address;
  temp.name = name(ANALOG_EVENT);
  temp.eventType = ANALOG_EVENT;
  temp.timeStamp = QDateTime::currentDateTime().toTime_t();
  temp.data.analog.num_channel = (int)(*data).size();
  for (unsigned int i = 0; i < (*data).size(); ++i)
  {
    temp.data.analog.channel[i] = (*data)[i];
  }
  this->EventQueue->Enqueue(temp);
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::newButtonValue(int state, int button)
{
  vtkVREventData temp;
  temp.connId = this->Address;
  temp.name = this->name(BUTTON_EVENT, button);
  temp.eventType = BUTTON_EVENT;
  temp.timeStamp = QDateTime::currentDateTime().toTime_t();
  temp.data.button.button = button;
  temp.data.button.state = state;
  this->EventQueue->Enqueue(temp);
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::newTrackerValue(vtkSmartPointer<vtkVRUITrackerState> data, int sensor)
{
  vtkVREventData temp;
  temp.connId = this->Address;
  temp.name = name(TRACKER_EVENT, sensor);
  temp.eventType = TRACKER_EVENT;
  temp.timeStamp = QDateTime::currentDateTime().toTime_t();
  temp.data.tracker.sensor = sensor;
  float rotMatrix[3][3];
  float pos[3];
  float q[4];
  data->GetPosition(pos);
  data->GetUnitQuaternion(q);

#if defined(VRUI_ENABLE_DEBUG) || 0
  cout << "pos=(" << pos[0] << "," << pos[1] << "," << pos[2] << ")" << endl;
  cout << "q=(" << q[0] << "," << q[1] << "," << q[2] << "," << q[3] << ")" << endl;
#endif

  // VTK expects quaternion in the format of (real, i, j, k), where as
  // FreeVR VRUI Daemon is giving us (i,j,k, real)
  float vtkQuat[4] = { q[3], q[0], q[1], q[2] };

  vtkMath::QuaternionToMatrix3x3(&vtkQuat[0], rotMatrix);

  this->Matrix->Element[0][0] = rotMatrix[0][0];
  this->Matrix->Element[1][0] = rotMatrix[1][0];
  this->Matrix->Element[2][0] = rotMatrix[2][0];
  this->Matrix->Element[3][0] = 0.0;

  this->Matrix->Element[0][1] = rotMatrix[0][1];
  this->Matrix->Element[1][1] = rotMatrix[1][1];
  this->Matrix->Element[2][1] = rotMatrix[2][1];
  // this->Matrix->Element[1][3] = pos[2]*1/12;
  this->Matrix->Element[3][1] = 0.0;

  this->Matrix->Element[0][2] = rotMatrix[0][2];
  this->Matrix->Element[1][2] = rotMatrix[1][2];
  this->Matrix->Element[2][2] = rotMatrix[2][2];
  // this->Matrix->Element[2][3] = pos[1]*-1/12;
  this->Matrix->Element[3][2] = 0.0;

  this->Matrix->Element[0][3] = pos[0] / 12.0;
  this->Matrix->Element[1][3] = pos[1] / 12.0;
  this->Matrix->Element[2][3] = pos[2] / 12.0;
  this->Matrix->Element[3][3] = 1.0f;

#if defined(VRUI_ENABLE_DEBUG) || 0
  if (sensor)
  {
    std::cout << "Pre multiplication matrix: " << std::endl;
    this->Matrix->PrintSelf(cout, vtkIndent(0));
  }
#endif

  this->ZUpToYUpMatrix->Element[0][0] = 1.0;
  this->ZUpToYUpMatrix->Element[1][0] = 0.0;
  this->ZUpToYUpMatrix->Element[2][0] = 0.0;
  this->ZUpToYUpMatrix->Element[3][0] = 0.0;

  this->ZUpToYUpMatrix->Element[0][1] = 0.0;
  this->ZUpToYUpMatrix->Element[1][1] = 0.0;
  this->ZUpToYUpMatrix->Element[2][1] = -1.0;
  this->ZUpToYUpMatrix->Element[3][1] = 0.0;

  this->ZUpToYUpMatrix->Element[0][2] = 0.0;
  this->ZUpToYUpMatrix->Element[1][2] = 1.0;
  this->ZUpToYUpMatrix->Element[2][2] = 0.0;
  this->ZUpToYUpMatrix->Element[3][2] = 0.0;

  this->ZUpToYUpMatrix->Element[0][3] = 0.0;
  this->ZUpToYUpMatrix->Element[1][3] = 0.0;
  this->ZUpToYUpMatrix->Element[2][3] = 0.0;
  this->ZUpToYUpMatrix->Element[3][3] = 1.0;

  vtkMatrix4x4::Multiply4x4(this->ZUpToYUpMatrix, this->Matrix, this->Matrix);

#if defined(VRUI_ENABLE_DEBUG) || 0
  if (sensor)
  {
    std::cout << "Post multiplication matrix: " << std::endl;
    this->Matrix->PrintSelf(cout, vtkIndent(0));
  }
#endif

#if defined(VRUI_ENABLE_DEBUG) || 0
  if (sensor)
    cout << "post pos=(" << this->Matrix->Element[0][3] << "," << this->Matrix->Element[1][3] << ","
         << this->Matrix->Element[2][3] << ")" << endl;
#endif

  temp.data.tracker.matrix[0] = this->Matrix->Element[0][0];
  temp.data.tracker.matrix[1] = this->Matrix->Element[0][1];
  temp.data.tracker.matrix[2] = this->Matrix->Element[0][2];
  temp.data.tracker.matrix[3] = this->Matrix->Element[0][3];

  temp.data.tracker.matrix[4] = this->Matrix->Element[1][0];
  temp.data.tracker.matrix[5] = this->Matrix->Element[1][1];
  temp.data.tracker.matrix[6] = this->Matrix->Element[1][2];
  temp.data.tracker.matrix[7] = this->Matrix->Element[1][3];

  temp.data.tracker.matrix[8] = this->Matrix->Element[2][0];
  temp.data.tracker.matrix[9] = this->Matrix->Element[2][1];
  temp.data.tracker.matrix[10] = this->Matrix->Element[2][2];
  temp.data.tracker.matrix[11] = this->Matrix->Element[2][3];

  temp.data.tracker.matrix[12] = this->Matrix->Element[3][0];
  temp.data.tracker.matrix[13] = this->Matrix->Element[3][1];
  temp.data.tracker.matrix[14] = this->Matrix->Element[3][2];
  temp.data.tracker.matrix[15] = this->Matrix->Element[3][3];

  this->EventQueue->Enqueue(temp);
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::getAndEnqueueButtonData()
{
  std::vector<bool>* buttons = this->Internals->State->GetButtonStates();
  for (unsigned int i = 0; i < (*buttons).size(); ++i)
  {
    newButtonValue((*buttons)[i], i);
  }
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::getAndEnqueueAnalogData()
{
  std::vector<float>* analog = this->Internals->State->GetValuatorStates();
  newAnalogValue(analog);
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::getAndEnqueueTrackerData()
{
  std::vector<vtkSmartPointer<vtkVRUITrackerState> >* trackers =
    this->Internals->State->GetTrackerStates();

  for (unsigned int i = 0; i < (*trackers).size(); ++i)
  {
    newTrackerValue((*trackers)[i], i);
  }
}

// ----------------------------------------------------------------------------
void pqVRUIConnection::setPort(std::string port)
{
  this->Port = port;
}
