/*=========================================================================

   Program: ParaView
   Module:  vtkVRPNConnection.cxx

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
#include "pqVRPNConnection.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "pqVRPNEventListener.h"
#include "pqView.h"

#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkVRPNCallBackHandlers.h"

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QMutexLocker>
#include <QtCore/QThread>

#include <vrpn_Analog.h>
#include <vrpn_Button.h>
#include <vrpn_Tracker.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>

pqVRPNEventListener* pqVRPNConnection::Listener = nullptr;

class pqVRPNConnection::pqInternals
{
public:
  pqInternals()
  {
    this->Tracker = 0;
    this->Button = 0;
    this->Analog = 0;
    this->Dial = 0;
    this->Text = 0;
  }

  ~pqInternals()
  {
    if (this->Tracker != 0)
    {
      delete this->Tracker;
    }
    if (this->Button != 0)
    {
      delete this->Button;
    }
    if (this->Analog != 0)
    {
      delete this->Analog;
    }
    if (this->Dial != 0)
    {
      delete this->Dial;
    }
    if (this->Text != 0)
    {
      delete this->Text;
    }
  }

  vrpn_Tracker_Remote* Tracker;
  vrpn_Button_Remote* Button;
  vrpn_Analog_Remote* Analog;
  vrpn_Dial_Remote* Dial;
  vrpn_Text_Receiver* Text;
};

// -----------------------------------------------------------------------cnstr
pqVRPNConnection::pqVRPNConnection(QObject* parentObject)
  : Superclass(parentObject)
{
  this->Internals = new pqInternals();
  if (!this->Listener)
  {
    this->Listener = new pqVRPNEventListener();
  }
  this->Initialized = false;
  this->Address = "";
  this->Name = "";
  this->Type = "VRPN";
  this->TrackerPresent = false;
  this->AnalogPresent = false;
  this->ButtonPresent = false;
  this->TrackerTransformPresent = false;
  this->Transformation = vtkMatrix4x4::New();
}

// -----------------------------------------------------------------------destr
pqVRPNConnection::~pqVRPNConnection()
{
  delete this->Internals;
  this->Transformation->Delete();
}

// ----------------------------------------------------------------------public
void pqVRPNConnection::addButton(std::string id, std::string name)
{
  std::stringstream returnStr;
  returnStr << "button." << id;
  this->ButtonMapping[returnStr.str()] = name;
  this->ButtonPresent = true;
}

// ----------------------------------------------------------------------public
void pqVRPNConnection::addAnalog(std::string id, std::string name)
{
  std::stringstream returnStr;
  returnStr << "analog." << id;
  this->AnalogMapping[returnStr.str()] = name;
  this->AnalogPresent = true;
}

// ----------------------------------------------------------------------public
void pqVRPNConnection::addTracking(std::string id, std::string name)
{
  std::stringstream returnStr;
  returnStr << "tracker." << id;
  this->TrackerMapping[returnStr.str()] = name;
  this->TrackerPresent = true;
}

// ----------------------------------------------------------------------public
void pqVRPNConnection::setName(std::string name)
{
  this->Name = name;
}

// ----------------------------------------------------------------------public
void pqVRPNConnection::setAddress(std::string address)
{
  this->Address = address;
}

// ----------------------------------------------------------------------public
void pqVRPNConnection::setQueue(vtkVRQueue* queue)
{
  this->EventQueue = queue;
}

// ----------------------------------------------------------------------------
bool pqVRPNConnection::init()
{
  if (this->Initialized)
  {
    return true;
  }

  this->Internals->Tracker = new vrpn_Tracker_Remote(this->Address.c_str());
  this->Internals->Analog = new vrpn_Analog_Remote(this->Address.c_str());
  this->Internals->Button = new vrpn_Button_Remote(this->Address.c_str());

  this->Internals->Tracker->register_change_handler(static_cast<void*>(this), handleTrackerChange);
  this->Internals->Analog->register_change_handler(static_cast<void*>(this), handleAnalogChange);
  this->Internals->Button->register_change_handler(static_cast<void*>(this), handleButtonChange);

  return this->Initialized = true;
}

// ----------------------------------------------------------------private-slot
void pqVRPNConnection::listen()
{
  if (this->Initialized)
  {
    this->Internals->Tracker->mainloop();
    this->Internals->Button->mainloop();
    this->Internals->Analog->mainloop();
  }
}

// ----------------------------------------------------------------private-slot
bool pqVRPNConnection::start()
{
  if (!this->Initialized)
  {
    return false;
  }

  this->Listener->addConnection(this);
  return true;
}

// ---------------------------------------------------------------------private
void pqVRPNConnection::stop()
{
  this->Listener->removeConnection(this);

  this->Initialized = false;
  delete this->Internals->Analog;
  this->Internals->Analog = nullptr;
  delete this->Internals->Button;
  this->Internals->Button = nullptr;
  delete this->Internals->Tracker;
  this->Internals->Tracker = nullptr;
}

// ---------------------------------------------------------------------private
void pqVRPNConnection::newAnalogValue(vrpn_ANALOGCB data)
{
  vtkVREvent event;
  event.connId = this->Address;
  event.name = name(ANALOG_EVENT);
  event.eventType = ANALOG_EVENT;
  event.timeStamp = QDateTime::currentDateTime().toTime_t();
  event.data.analog.num_channels = data.num_channel;
  for (int i = 0; i < data.num_channel; ++i)
  {
    event.data.analog.channel[i] = data.channel[i];
  }
  this->EventQueue->Enqueue(event);
}

// ---------------------------------------------------------------------private
void pqVRPNConnection::newButtonValue(vrpn_BUTTONCB data)
{
  vtkVREvent event;
  event.connId = this->Address;
  event.name = this->name(BUTTON_EVENT, data.button);
  event.eventType = BUTTON_EVENT;
  event.timeStamp = QDateTime::currentDateTime().toTime_t();
  event.data.button.button = data.button;
  event.data.button.state = data.state;
  this->EventQueue->Enqueue(event);
}

// ---------------------------------------------------------------------private
void pqVRPNConnection::newTrackerValue(vrpn_TRACKERCB data)
{
  vtkVREvent event;
  event.connId = this->Address;
  event.name = name(TRACKER_EVENT, data.sensor);
  event.eventType = TRACKER_EVENT;
  event.timeStamp = QDateTime::currentDateTime().toTime_t();
  event.data.tracker.sensor = data.sensor;
  double rotMatrix[3][3];
  double vtkQuat[4] = { data.quat[3], data.quat[0], data.quat[1], data.quat[2] };
  vtkMath::QuaternionToMatrix3x3(vtkQuat, rotMatrix);
  vtkMatrix4x4* matrix = vtkMatrix4x4::New();
#define COLUMN_MAJOR 1
#if COLUMN_MAJOR
  matrix->Element[0][0] = rotMatrix[0][0];
  matrix->Element[0][1] = rotMatrix[0][1];
  matrix->Element[0][2] = rotMatrix[0][2];
  matrix->Element[0][3] = data.pos[0] * 1;

  matrix->Element[1][0] = rotMatrix[1][0];
  matrix->Element[1][1] = 1 * rotMatrix[1][1];
  matrix->Element[1][2] = rotMatrix[1][2];
  matrix->Element[1][3] = data.pos[1];

  matrix->Element[2][0] = rotMatrix[2][0];
  matrix->Element[2][1] = rotMatrix[2][1];
  matrix->Element[2][2] = rotMatrix[2][2];
  matrix->Element[2][3] = data.pos[2];

  matrix->Element[3][0] = 0.0f;
  matrix->Element[3][1] = 0.0f;
  matrix->Element[3][2] = 0.0f;
  matrix->Element[3][3] = 1.0f;

#else

  matrix->Element[0][0] = rotMatrix[0][0];
  matrix->Element[1][0] = rotMatrix[0][1];
  matrix->Element[2][0] = rotMatrix[0][2];
  matrix->Element[3][0] = 0.0;

  matrix->Element[0][1] = rotMatrix[1][0];
  matrix->Element[1][1] = rotMatrix[1][1];
  matrix->Element[2][1] = rotMatrix[1][2];
  matrix->Element[3][1] = 0.0;

  matrix->Element[0][2] = rotMatrix[2][0];
  matrix->Element[1][2] = rotMatrix[2][1];
  matrix->Element[2][2] = rotMatrix[2][2];
  matrix->Element[3][2] = 0.0;

  matrix->Element[0][3] = data.pos[0] * 1;
  matrix->Element[1][3] = data.pos[1];
  matrix->Element[2][3] = data.pos[2];
  matrix->Element[3][3] = 1.0f;
#endif

  vtkMatrix4x4::Multiply4x4(this->Transformation, matrix, matrix);

#if COLUMN_MAJOR
  event.data.tracker.matrix[0] = matrix->Element[0][0];
  event.data.tracker.matrix[1] = matrix->Element[0][1];
  event.data.tracker.matrix[2] = matrix->Element[0][2];
  event.data.tracker.matrix[3] = matrix->Element[0][3];

  event.data.tracker.matrix[4] = matrix->Element[1][0];
  event.data.tracker.matrix[5] = matrix->Element[1][1];
  event.data.tracker.matrix[6] = matrix->Element[1][2];
  event.data.tracker.matrix[7] = matrix->Element[1][3];

  event.data.tracker.matrix[8] = matrix->Element[2][0];
  event.data.tracker.matrix[9] = matrix->Element[2][1];
  event.data.tracker.matrix[10] = matrix->Element[2][2];
  event.data.tracker.matrix[11] = matrix->Element[2][3];

  event.data.tracker.matrix[12] = matrix->Element[3][0];
  event.data.tracker.matrix[13] = matrix->Element[3][1];
  event.data.tracker.matrix[14] = matrix->Element[3][2];
  event.data.tracker.matrix[15] = matrix->Element[3][3];
#else
  event.data.tracker.matrix[0] = matrix->Element[0][0];
  event.data.tracker.matrix[1] = matrix->Element[1][0];
  event.data.tracker.matrix[2] = matrix->Element[2][0];
  event.data.tracker.matrix[3] = matrix->Element[3][0];

  event.data.tracker.matrix[4] = matrix->Element[0][1];
  event.data.tracker.matrix[5] = matrix->Element[1][1];
  event.data.tracker.matrix[6] = matrix->Element[2][1];
  event.data.tracker.matrix[7] = matrix->Element[3][1];

  event.data.tracker.matrix[8] = matrix->Element[0][2];
  event.data.tracker.matrix[9] = matrix->Element[1][2];
  event.data.tracker.matrix[10] = matrix->Element[2][2];
  event.data.tracker.matrix[11] = matrix->Element[3][2];

  event.data.tracker.matrix[12] = matrix->Element[0][3];
  event.data.tracker.matrix[13] = matrix->Element[1][3];
  event.data.tracker.matrix[14] = matrix->Element[2][3];
  event.data.tracker.matrix[15] = matrix->Element[3][3];
#endif
  matrix->Delete();
  this->EventQueue->Enqueue(event);
}

// ---------------------------------------------------------------------private
std::string pqVRPNConnection::name(int eventType, int id)
{
  std::stringstream returnStr, eventStr;
  if (this->Name.size())
    returnStr << this->Name << ".";
  else
    returnStr << this->Address << ".";
  switch (eventType)
  {
    case ANALOG_EVENT:
      eventStr << "analog." << id;
      if (this->AnalogMapping.find(eventStr.str()) != this->AnalogMapping.end())
        returnStr << this->AnalogMapping[eventStr.str()];
      else
        returnStr << eventStr.str();
      break;
    case BUTTON_EVENT:
      eventStr << "button." << id;
      if (this->ButtonMapping.find(eventStr.str()) != this->ButtonMapping.end())
        returnStr << this->ButtonMapping[eventStr.str()];
      else
        returnStr << eventStr.str();
      break;
    case TRACKER_EVENT:
      eventStr << "tracker." << id;
      if (this->TrackerMapping.find(eventStr.str()) != this->TrackerMapping.end())
        returnStr << this->TrackerMapping[eventStr.str()];
      else
        returnStr << eventStr.str();
      break;
  }
  return returnStr.str();
}

// ---------------------------------------------------------------------private
void pqVRPNConnection::verifyConfig(const char* id, const char* name)
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

// ----------------------------------------------------------------------public
bool pqVRPNConnection::configure(vtkPVXMLElement* child, vtkSMProxyLocator*)
{
  bool returnVal = false;
  if (child->GetName() && strcmp(child->GetName(), "VRPNConnection") == 0)
  {
    for (unsigned neCount = 0; neCount < child->GetNumberOfNestedElements(); ++neCount)
    {
      vtkPVXMLElement* nestedElement = child->GetNestedElement(neCount);
      if (nestedElement && nestedElement->GetName())
      {
        const char* id = nestedElement->GetAttributeOrEmpty("id");
        const char* name = nestedElement->GetAttributeOrEmpty("name");
        this->verifyConfig(id, name);

        if (strcmp(nestedElement->GetName(), "Button") == 0)
        {
          this->addButton(id, name);
        }
        else if (strcmp(nestedElement->GetName(), "Analog") == 0)
        {
          this->addAnalog(id, name);
        }
        else if (strcmp(nestedElement->GetName(), "Tracker") == 0)
        {
          this->addTracking(id, name);
        }
        else if (strcmp(nestedElement->GetName(), "TrackerTransform") == 0)
        {
          this->configureTransform(nestedElement);
        }

        else
        {
          qWarning() << "Unknown Device type: \"" << nestedElement->GetName() << "\"";
        }
        returnVal = true;
      }
    }
  }
  return returnVal;
}

// ---------------------------------------------------------------------private
void pqVRPNConnection::configureTransform(vtkPVXMLElement* child)
{
  if (child->GetName() && strcmp(child->GetName(), "TrackerTransform") == 0)
  {
    child->GetVectorAttribute("value", 16, (double*)this->Transformation->Element);
    this->TrackerTransformPresent = true;
  }
}

// ----------------------------------------------------------------------public
vtkPVXMLElement* pqVRPNConnection::saveConfiguration() const
{
  vtkPVXMLElement* child = vtkPVXMLElement::New();
  child->SetName("VRPNConnection");
  child->AddAttribute("name", this->Name.c_str());
  child->AddAttribute("address", this->Address.c_str());
  saveButtonEventConfig(child);
  saveAnalogEventConfig(child);
  saveTrackerEventConfig(child);
  saveTrackerTransformationConfig(child);
  return child;
}

// ----------------------------------------------------------------------public
void pqVRPNConnection::saveButtonEventConfig(vtkPVXMLElement* child) const
{
  if (!this->ButtonPresent)
    return;
  for (std::map<std::string, std::string>::const_iterator iter = this->ButtonMapping.begin();
       iter != this->ButtonMapping.end(); ++iter)
  {
    std::string key = iter->first;
    std::string value = iter->second;
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
    vtkPVXMLElement* element = vtkPVXMLElement::New();
    if (strcmp(token[0].c_str(), "button") == 0)
    {
      element->SetName("Button");
      element->AddAttribute("id", token[1].c_str());
      element->AddAttribute("name", value.c_str());
    }
    child->AddNestedElement(element);
    element->FastDelete();
  }
}

// ----------------------------------------------------------------------public
void pqVRPNConnection::saveAnalogEventConfig(vtkPVXMLElement* child) const
{
  if (!this->AnalogPresent)
    return;
  for (std::map<std::string, std::string>::const_iterator iter = this->AnalogMapping.begin();
       iter != this->AnalogMapping.end(); ++iter)
  {
    std::string key = iter->first;
    std::string value = iter->second;
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
    vtkPVXMLElement* element = vtkPVXMLElement::New();
    if (strcmp(token[0].c_str(), "analog") == 0)
    {
      element->SetName("Analog");
      element->AddAttribute("id", token[1].c_str());
      element->AddAttribute("name", value.c_str());
    }
    child->AddNestedElement(element);
    element->FastDelete();
  }
}

// ----------------------------------------------------------------------public
void pqVRPNConnection::saveTrackerEventConfig(vtkPVXMLElement* child) const
{
  if (!this->TrackerPresent)
    return;
  for (std::map<std::string, std::string>::const_iterator iter = this->TrackerMapping.begin();
       iter != this->TrackerMapping.end(); ++iter)
  {
    std::string key = iter->first;
    std::string value = iter->second;
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
    vtkPVXMLElement* element = vtkPVXMLElement::New();
    if (strcmp(token[0].c_str(), "tracker") == 0)
    {
      element->SetName("Tracker");
      element->AddAttribute("id", token[1].c_str());
      element->AddAttribute("name", value.c_str());
    }
    child->AddNestedElement(element);
    element->FastDelete();
  }
}

// ---------------------------------------------------------------------private
void pqVRPNConnection::saveTrackerTransformationConfig(vtkPVXMLElement* child) const
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

// ----------------------------------------------------------------------public
void pqVRPNConnection::setTransformation(vtkMatrix4x4* matrix)
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
