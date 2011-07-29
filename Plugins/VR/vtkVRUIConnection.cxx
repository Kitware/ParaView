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
#include "vtkVRUIConnection.h"

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
#include <QDateTime>
#include <QDebug>
#include <vtkstd/vector>
#include <QTcpSocket>
#include <iostream>
#include <sstream>
#include <algorithm>
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkMath.h"
#include "vtkVRUIPipe.h"
#include "vtkVRUIServerState.h"
#include "vtkVRUITrackerState.h"


class vtkVRUIConnection::pqInternals
{
    public:
  pqInternals()
  {
    this->Active=false;
    this->Pipe=0;
    this->State=0;
    this->StateMutex=0;
    this->Streaming=false;           // streaming
    this->PacketSignalCond=0;        // for streaming
    this->PacketSignalCondMutex=0; // for streaming
  }

  ~pqInternals()
  {
    if(this->Pipe!=0)
      {
      delete this->Pipe;
      }
    if(this->State!=0)
      {
      delete this->State;
      }
  }

  bool Active;
  vtkVRUIPipe *Pipe;
  vtkVRUIServerState *State;
  QMutex *StateMutex;
  bool Streaming;                   // streaming
  QWaitCondition *PacketSignalCond; // for streaming
  QMutex *PacketSignalCondMutex;    // for streaming


  // Streaming routine called when streaming is used
  void stream()
  {
    bool done=false;
    QMutex *stateLock;
    while(!done)
      {
      vtkVRUIPipe::MessageTag m=this->Pipe->Receive();
      switch(m)
        {
        case vtkVRUIPipe::PACKET_REPLY:
          cout << "thread:PACKET_REPLY ok : tag=" << m << endl;
          this->StateMutex->lock();
          this->Pipe->ReadState(this->State);
          this->StateMutex->unlock();
          break;
        case vtkVRUIPipe::STOPSTREAM_REPLY:
          cout << "thread:STOPSTREAM_REPLY ok : tag=" << m << endl;
          done=true;
          break;
        default:
          cerr << "thread: Mismatching message while waiting for PACKET_REPLY: tag=" << m << endl;
          done=true;
          break;
        }
      }
  }

};


// -----------------------------------------------------------------------cnstr
vtkVRUIConnection::vtkVRUIConnection(QObject* parentObject)
  :Superclass( parentObject )
{
  this->Internals=new pqInternals();
  this->Initialized=false;
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
}

// -----------------------------------------------------------------------destr
vtkVRUIConnection::~vtkVRUIConnection()
{
  this->StopStream();

  this->Deactivate();

  delete this->Internals;
  this->Transformation->Delete();
}

// ----------------------------------------------------------------------public
void vtkVRUIConnection::AddButton(std::string id, std::string name)
{
  std::stringstream returnStr;
  returnStr << "button." << id;
  this->ButtonMapping[returnStr.str()] = name;
  this->ButtonPresent = true;
}

// ----------------------------------------------------------------------public
void vtkVRUIConnection::AddAnalog(std::string id, std::string name )
{
  std::stringstream returnStr;
  returnStr << "analog." << id;
  this->AnalogMapping[returnStr.str()] = name;
  this->AnalogPresent = true;
}

// ----------------------------------------------------------------------public
void vtkVRUIConnection::AddTracking( std::string id, std::string name)
{
  std::stringstream returnStr;
  returnStr << "tracker." << id;
  this->TrackerMapping[returnStr.str()] = name;
  this->TrackerPresent = true;
}

// ----------------------------------------------------------------------public
void vtkVRUIConnection::SetName(std::string name)
{
  this->Name = name;
}

// ----------------------------------------------------------------------public
void vtkVRUIConnection::SetAddress(std::string name)
{
  this->Address = name;
}

// ----------------------------------------------------------------------public
void vtkVRUIConnection::SetQueue( vtkVRQueue* queue )
{
  this->EventQueue = queue;
}

// ----------------------------------------------------------------------------
bool vtkVRUIConnection::Init()
{
  QTcpSocket *socket=new QTcpSocket;
  socket->connectToHost(QString(this->Address.c_str()),atoi( this->Port.c_str() )); // ReadWrite?
  this->Internals->Pipe=new vtkVRUIPipe(socket);
  this->Internals->Pipe->Send(vtkVRUIPipe::CONNECT_REQUEST);
  if(!this->Internals->Pipe->WaitForServerReply(30000)) // 30s
    {
    cerr << "Timeout while waiting for CONNECT_REPLY" << endl;
    delete this->Internals->Pipe;
    this->Internals->Pipe=0;
    return false;
    }
  if(this->Internals->Pipe->Receive()!=vtkVRUIPipe::CONNECT_REPLY)
    {
    cerr << "Mismatching message while waiting for CONNECT_REPLY" << endl;
    delete this->Internals->Pipe;
    this->Internals->Pipe=0;
    return false;
    }

  this->Internals->State=new vtkVRUIServerState;
  this->Internals->StateMutex=new QMutex;

  this->Internals->Pipe->ReadLayout(this->Internals->State);

  this->Activate();

  //this->StartStream();

  this->Initialized=true;
  return true;
}

// ----------------------------------------------------------------private-slot
void vtkVRUIConnection::run()
{
  while ( !this->_Stop )
    {
    if(this->Initialized)
      {
      if ( this->Internals->Streaming )
        {
        this->Internals->stream();
        }
      else
        {
        this->callback();
        }
      }
    }
}

// ---------------------------------------------------------------------private
void vtkVRUIConnection::Stop()
{
  this->_Stop = true;
  this->StopStream();
  this->Deactivate();
  QThread::terminate();
}


// ---------------------------------------------------------------------private
std::string vtkVRUIConnection::GetName( int eventType, int id )
{
  std::stringstream returnStr,connection,event;
  if(this->Name.size())
    returnStr << this->Name << ".";
  else
    returnStr << this->Address << ".";
  switch (eventType )
    {
    case ANALOG_EVENT:
      event << "analog." << id;
      if( this->AnalogMapping.find( event.str())!= this->ButtonMapping.end())
        returnStr << this->AnalogMapping[event.str()];
      else
        returnStr << event.str();
      break;
    case BUTTON_EVENT:
      event << "button." << id;
      if( this->ButtonMapping.find( event.str())!= this->ButtonMapping.end())
        returnStr << this->ButtonMapping[event.str()];
      else
        returnStr << event.str();
      break;
    case TRACKER_EVENT:
      event << "tracker."<< id;
      if( this->TrackerMapping.find( event.str())!=this->TrackerMapping.end())
        returnStr << this->TrackerMapping[event.str()];
      else
        returnStr << event.str();
      break;
    }
  return returnStr.str();
}

// ---------------------------------------------------------------------private
void vtkVRUIConnection::verifyConfig( const char* id,
                                      const char* name )
{
  if ( !id )
    {
    qWarning() << "\"id\" should be specified";
    }
  if ( !name )
    {
    qWarning() << "\"name\" should be specified";
    }
}

// ----------------------------------------------------------------------public
bool vtkVRUIConnection::configure(vtkPVXMLElement* child, vtkSMProxyLocator*)
{
  bool returnVal = false;
  if (child->GetName() && strcmp(child->GetName(),"VRUIConnection") == 0 )
    {
    for ( unsigned cc=0; cc < child->GetNumberOfNestedElements();++cc )
      {
      vtkPVXMLElement* event = child->GetNestedElement(cc);
      if ( event && event->GetName() )
        {
        const char* id = event->GetAttributeOrEmpty( "id" );
        const char* name = event->GetAttributeOrEmpty( "name" );
        this->verifyConfig(id, name);

        if ( strcmp( event->GetName(), "Button" )==0 )
          {
          this->AddButton( id, name );
          }
        else if ( strcmp( event->GetName(), "Analog" )==0 )
          {
          this->AddAnalog( id, name );
          }
        else if ( strcmp ( event->GetName(),  "Tracker" ) ==0 )
          {
          this->AddTracking( id, name );
          }
        else if ( strcmp ( event->GetName(),  "TrackerTransform" ) ==0 )
          {
          this->configureTransform( event );
          }

        else
          {
          qWarning() << "Unknown Device type: \"" << event->GetName() <<"\"";
          }
        returnVal = true;
        }
      }
    }
  return returnVal;
}

// ---------------------------------------------------------------------private
void vtkVRUIConnection::configureTransform( vtkPVXMLElement* child )
{
  if (child->GetName() && strcmp( child->GetName(), "TrackerTransform" )==0 )
    {
    child->GetVectorAttribute( "value",
                               16,
                               ( double* ) this->Transformation->Element );
    this->TrackerTransformPresent = true;
    }
}

// ----------------------------------------------------------------------public
vtkPVXMLElement* vtkVRUIConnection::saveConfiguration() const
{
  vtkPVXMLElement* child = vtkPVXMLElement::New();
  child->SetName("VRUIConnection");
  child->AddAttribute( "name", this->Name.c_str() );
  child->AddAttribute( "address",  this->Address.c_str() );
  child->AddAttribute( "port", this->Port.c_str() );
  saveButtonEventConfig( child );
  saveAnalogEventConfig( child );
  saveTrackerEventConfig( child );
  saveTrackerTransformationConfig( child );
  return child;
}

// ----------------------------------------------------------------------public
void vtkVRUIConnection::saveButtonEventConfig( vtkPVXMLElement* child )const
{
  if(!this->ButtonPresent) return;
  for ( std::map<std::string,std::string>::const_iterator it = this->ButtonMapping.begin();
        it!=this->ButtonMapping.end();
        ++it )
    {
    std::string key = it->first;
    std::string value = it->second;
    std::replace(key.begin(), key.end(), '.', ' ');
    std::istringstream stm(key);
    std::vector<std::string> token;
    for (;;)
      {
      std::string word;
      if (!(stm >> word)) break;
      token.push_back(word);
      }
    vtkPVXMLElement* event = vtkPVXMLElement::New();
    if ( strcmp( token[0].c_str(), "button" )==0 )
      {
      event->SetName("Button");
      event->AddAttribute("id", token[1].c_str() );
      event->AddAttribute("name",value.c_str());
      }
    child->AddNestedElement(event);
    event->FastDelete();
    }
}

// ----------------------------------------------------------------------public
void vtkVRUIConnection::saveAnalogEventConfig( vtkPVXMLElement* child ) const
{
  if(!this->AnalogPresent) return;
  for ( std::map<std::string,std::string>::const_iterator it = this->AnalogMapping.begin();
        it!=this->AnalogMapping.end();
        ++it )
    {
    std::string key = it->first;
    std::string value = it->second;
    std::replace(key.begin(), key.end(), '.', ' ');
    std::istringstream stm(key);
    std::vector<std::string> token;
    for (;;)
      {
      std::string word;
      if (!(stm >> word)) break;
      token.push_back(word);
      }
    vtkPVXMLElement* event = vtkPVXMLElement::New();
    if ( strcmp( token[0].c_str(), "analog" )==0 )
      {
      event->SetName("Analog");
      event->AddAttribute("id", token[1].c_str() );
      event->AddAttribute("name",value.c_str());
      }
    child->AddNestedElement(event);
    event->FastDelete();
    }
}

// ----------------------------------------------------------------------public
void vtkVRUIConnection::saveTrackerEventConfig( vtkPVXMLElement* child ) const
{
  if(!this->TrackerPresent) return;
  for ( std::map<std::string,std::string>::const_iterator it = this->TrackerMapping.begin();
        it!=this->TrackerMapping.end();
        ++it )
    {
    std::string key = it->first;
    std::string value = it->second;
    std::replace(key.begin(), key.end(), '.', ' ');
    std::istringstream stm(key);
    std::vector<std::string> token;
    for (;;)
      {
      std::string word;
      if (!(stm >> word)) break;
      token.push_back(word);
      }
    vtkPVXMLElement* event = vtkPVXMLElement::New();
    if ( strcmp( token[0].c_str(), "tracker" )==0 )
      {
      event->SetName("Tracker");
      event->AddAttribute("id", token[1].c_str() );
      event->AddAttribute("name",value.c_str());
      }
    child->AddNestedElement(event);
    event->FastDelete();
    }
}

// ---------------------------------------------------------------------private
void vtkVRUIConnection::saveTrackerTransformationConfig( vtkPVXMLElement* child ) const
{
  if(!this->TrackerTransformPresent) return;
  vtkPVXMLElement* transformationMatrix = vtkPVXMLElement::New();
  transformationMatrix->SetName("TrackerTransform");
  std::stringstream matrix;
  for (int i = 0; i < 16; ++i)
    {
    matrix <<  double( *( ( double* )this->Transformation->Element +i ) ) << " ";
    }
  transformationMatrix->AddAttribute( "value",  matrix.str().c_str() );
  child->AddNestedElement(transformationMatrix);
  transformationMatrix->FastDelete();
}

// ----------------------------------------------------------------------public
void vtkVRUIConnection::SetTransformation( vtkMatrix4x4* matrix )
{
  for (int i = 0; i < 4; ++i)
    {
    for (int j = 0; j < 4; ++j)
      {
      this->Transformation->SetElement(i,j, matrix->GetElement( i,j ) );
      }
    }
  this->TrackerTransformPresent = true;
}

void vtkVRUIConnection::Activate()
{
  if(!this->Internals->Active)
    {
    this->Internals->Pipe->Send(vtkVRUIPipe::ACTIVATE_REQUEST);
    this->Internals->Active=true;
    }
}

void vtkVRUIConnection::Deactivate()
{
  if(this->Internals->Active)
    {
    this->Internals->Active=false;
    this->Internals->Pipe->Send(vtkVRUIPipe::DEACTIVATE_REQUEST);
    }
}

void vtkVRUIConnection::StartStream()
{
  if(this->Internals->Active)
    {
    this->Internals->Streaming=true;
    this->Internals->PacketSignalCond=new QWaitCondition;
    QMutex m;
    m.lock();
    this->Internals->Pipe->Send(vtkVRUIPipe::STARTSTREAM_REQUEST);
    this->Internals->PacketSignalCond->wait(&m);
    m.unlock();
    }
}

void vtkVRUIConnection::StopStream()
{
  if(this->Internals->Streaming)
    {
    this->Internals->Streaming=false;
    this->Internals->Pipe->Send(vtkVRUIPipe::STOPSTREAM_REQUEST);
    }
}

void vtkVRUIConnection::callback()
{
  if(this->Initialized)
    {
    // std::cout << "callback()" << std::endl;

    this->Internals->StateMutex->lock();
    this->GetAndEnqueueButtonData();
    this->GetAndEnqueueAnalogData();
    this->GetAndEnqueueTrackerData();
    this->Internals->StateMutex->unlock();
    this->GetNextPacket(); // for the next step
    }
}

void vtkVRUIConnection::GetNextPacket()
{
  if(this->Internals->Active)
    {
    if(this->Internals->Streaming)
      {
      // With a thread
      this->Internals->PacketSignalCondMutex->lock();
      this->Internals->PacketSignalCond->wait(this->Internals->PacketSignalCondMutex);
      this->Internals->PacketSignalCondMutex->unlock();
      }
    else
      {
      // With a loop
      this->Internals->Pipe->Send(vtkVRUIPipe::PACKET_REQUEST);
      if(this->Internals->Pipe->WaitForServerReply(10000))
        {
        if(this->Internals->Pipe->Receive()!=vtkVRUIPipe::PACKET_REPLY)
          {
          cout << "VRUI Mismatching message while waiting for PACKET_REPLY" << endl;
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

void vtkVRUIConnection::PrintPositionOrientation()
{
  vtkstd::vector<vtkSmartPointer<vtkVRUITrackerState> > *trackers=
    this->Internals->State->GetTrackerStates();

  float pos[3];
  float q[4];
  (*trackers)[0]->GetPosition(pos);
  (*trackers)[0]->GetUnitQuaternion(q);

  // cout << "pos=("<< pos[0] << "," << pos[1] << "," << pos[2] << ")" << endl;
  // cout << "q=("<< q[0] << "," << q[1] << "," << q[2] << "," << q[3] << ")"
  //      << endl;

  vtkstd::vector<bool> *buttons=this->Internals->State->GetButtonStates();
  // cout << "button0=" << (*buttons)[0] << endl;
  pqView *view = 0;
  view = pqActiveObjects::instance().activeView();
  if ( view )
    {
    vtkSMRenderViewProxy *proxy = 0;
    proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );
    if ( proxy )
      {
      double rotMat[3][3];
      vtkMath::QuaternionToMatrix3x3((double*)q,rotMat);
      vtkSMDoubleVectorProperty *prop = 0;
      prop = vtkSMDoubleVectorProperty::SafeDownCast( proxy->GetProperty( "HeadPose" ) );
      if ( prop )
        {
        prop->SetElement( 0,  rotMat[0][0] );
        prop->SetElement( 1,  rotMat[0][1] );
        prop->SetElement( 2,  rotMat[0][2] );
        prop->SetElement( 3,  pos [0]*1  );

        prop->SetElement( 4,  rotMat[1][0] );
        prop->SetElement( 5,  rotMat[1][1] );
        prop->SetElement( 6,  rotMat[1][2] );
        prop->SetElement( 7,  pos [1]*1  );

        prop->SetElement( 8,  rotMat[2][0] );
        prop->SetElement( 9,  rotMat[2][1] );
        prop->SetElement( 10, rotMat[2][2] );
        prop->SetElement( 11, pos [2]*1  );

        prop->SetElement( 12, 0.0 );
        prop->SetElement( 13, 0.0 );
        prop->SetElement( 14, 0.0 );
        prop->SetElement( 15, 1.0 );

        // proxy->SetHeadPose( rotMat[0][0], rotMat[0][1],rotMat[0][2], pos[0]*1,
        //                  rotMat[1][0], rotMat[1][1],rotMat[1][2], pos[1]*1,
        //                  rotMat[2][0], rotMat[2][1],rotMat[2][2], pos[2]*1,
        //                  0.0, 0.0, 0.0, 1.0 );
        proxy->UpdateVTKObjects();
        proxy->StillRender();
        }
      }
    }
}

void vtkVRUIConnection::NewAnalogValue(vtkstd::vector<float> *data)
{
  vtkVREventData temp;
  temp.connId = this->Address;
  temp.name = GetName( ANALOG_EVENT);
  temp.eventType = ANALOG_EVENT;
  temp.timeStamp =   QDateTime::currentDateTime().toTime_t();
  temp.data.analog.num_channel = ( *data ).size();
  for ( int i=0 ; i<( *data ).size() ;++i )
    {
    temp.data.analog.channel[i] = ( *data )[i];
    }
  this->EventQueue->enqueue( temp );
}

void vtkVRUIConnection::NewButtonValue(int state,  int button)
{
  vtkVREventData temp;
  temp.connId = this->Address;
  temp.name = this->GetName( BUTTON_EVENT, button );
  temp.eventType = BUTTON_EVENT;
  temp.timeStamp =   QDateTime::currentDateTime().toTime_t();
  temp.data.button.button = button;
  temp.data.button.state = state;
  this->EventQueue->enqueue( temp );
}

void vtkVRUIConnection::NewTrackerValue(vtkSmartPointer<vtkVRUITrackerState> data, int sensor)
{

  vtkVREventData temp;
  temp.connId = this->Address;
  temp.name = GetName( TRACKER_EVENT, sensor );
  temp.eventType = TRACKER_EVENT;
  temp.timeStamp =   QDateTime::currentDateTime().toTime_t();
  temp.data.tracker.sensor = sensor;
  float rotMatrix[3][3];
  float pos[3];
  float q[4];
  data->GetPosition(pos);
  data->GetUnitQuaternion(q);
  vtkMath::QuaternionToMatrix3x3(&q[0], rotMatrix );
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();

  matrix->Element[0][0] = rotMatrix[0][0];
  matrix->Element[0][1] = rotMatrix[0][1];
  matrix->Element[0][2] = rotMatrix[0][2];
  matrix->Element[0][3] = pos[0];

  matrix->Element[1][0] = rotMatrix[1][0];
  matrix->Element[1][1] = 1*rotMatrix[1][1];
  matrix->Element[1][2] = rotMatrix[1][2];
  matrix->Element[1][3] = pos[1];

  matrix->Element[2][0] = rotMatrix[2][0];
  matrix->Element[2][1] = rotMatrix[2][1];
  matrix->Element[2][2] = rotMatrix[2][2];
  matrix->Element[2][3] = pos[2];

  matrix->Element[3][0] = 0.0f;
  matrix->Element[3][1] = 0.0f;
  matrix->Element[3][2] = 0.0f;
  matrix->Element[3][3] = 1.0f;

  vtkMatrix4x4::Multiply4x4( this->Transformation, matrix, matrix );

  temp.data.tracker.matrix[0] = matrix->Element[0][0];
  temp.data.tracker.matrix[1] = matrix->Element[0][1];
  temp.data.tracker.matrix[2] = matrix->Element[0][2];
  temp.data.tracker.matrix[3] = matrix->Element[0][3];

  temp.data.tracker.matrix[4] = matrix->Element[1][0];
  temp.data.tracker.matrix[5] = matrix->Element[1][1];
  temp.data.tracker.matrix[6] = matrix->Element[1][2];
  temp.data.tracker.matrix[7] = matrix->Element[1][3];

  temp.data.tracker.matrix[8] = matrix->Element[2][0];
  temp.data.tracker.matrix[9] = matrix->Element[2][1];
  temp.data.tracker.matrix[10] = matrix->Element[2][2];
  temp.data.tracker.matrix[11] = matrix->Element[2][3];

  temp.data.tracker.matrix[12] = matrix->Element[3][0];
  temp.data.tracker.matrix[13] = matrix->Element[3][1];
  temp.data.tracker.matrix[14] = matrix->Element[3][2];
  temp.data.tracker.matrix[15] = matrix->Element[3][3];

  matrix->Delete();
  this->EventQueue->enqueue( temp );
}

void vtkVRUIConnection::GetAndEnqueueButtonData()
{
  vtkstd::vector<bool> *buttons=this->Internals->State->GetButtonStates();
  for (int i = 0; i < ( *buttons ).size(); ++i)
    {
    NewButtonValue( ( *buttons )[i], i );
    }
}

void vtkVRUIConnection::GetAndEnqueueAnalogData()
{
  vtkstd::vector<float> *analog=this->Internals->State->GetValuatorStates();
  NewAnalogValue( analog );
}

void vtkVRUIConnection::GetAndEnqueueTrackerData()
{
  vtkstd::vector<vtkSmartPointer<vtkVRUITrackerState> > *trackers=
    this->Internals->State->GetTrackerStates();

  for (int i = 0; i < ( *trackers ).size(); ++i)
    {
    NewTrackerValue( (*trackers )[i] , i);
    }
}

void vtkVRUIConnection::SetPort(std::string port)
{
  this->Port = port;
}
