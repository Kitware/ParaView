/*=========================================================================

   Program: ParaView
   Module:    ParaViewVRPN.h

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
#ifndef pqVRPNConnection_h
#define pqVRPNConnection_h

#include <QtCore/QObject>

#include "vtkMatrix4x4.h"
#include "vtkTransform.h"
#include "vtkVRQueue.h"

#include <QtCore/QMutex>

#include <vrpn_Analog.h>
#include <vrpn_Button.h>
#include <vrpn_Tracker.h>

#include <map>

class pqVRPNEventListener;

class vtkPVXMLElement;
class vtkSMProxyLocator;

/// Callback to listen to VRPN events
class pqVRPNConnection : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  explicit pqVRPNConnection(QObject* parent = 0);
  virtual ~pqVRPNConnection();

  // Description:
  // Address of the device. For example, "Tracker0@localhost"
  void setAddress(std::string address);

  // Description:
  // Address of the device. For example, "Tracker0@localhost"
  std::string address() { return this->Address; }

  // Description:
  // Set the device name.
  void setName(std::string name);

  // Description:
  // Get the device name.
  std::string name() { return this->Name; }

  // Description:
  // Add button device
  void addButton(std::string id, std::string name);

  // Description:
  // Add Analog device
  void addAnalog(std::string id, std::string name);

  // Description:
  // Add tracking device
  void addTracking(std::string id, std::string name);

  // Description:
  // Adding a transformation matrix
  void setTransformation(vtkMatrix4x4* matrix);

  // Description:
  // Initialize the device with the name.
  bool init();

  // Description:
  // Register this connection with the event listener and request that the
  // event listener start.
  bool start();

  // Description:
  // Terminate the thread
  void stop();

  // Description:
  // Sets the Event Queue into which the vrpn data needs to be written
  void setQueue(vtkVRQueue* queue);

  /// configure the style using the xml configuration.
  virtual bool configure(vtkPVXMLElement* child, vtkSMProxyLocator*);

  /// save the xml configuration.
  virtual vtkPVXMLElement* saveConfiguration() const;

  /// Access to analog map
  std::map<std::string, std::string> analogMap() { return this->AnalogMapping; }
  /// Access to analog map
  void setAnalogMap(const std::map<std::string, std::string>& m)
  {
    this->AnalogMapping = m;
    this->AnalogPresent = (this->AnalogMapping.size() > 0);
  }

  /// Access to button map
  std::map<std::string, std::string> buttonMap() { return this->ButtonMapping; }
  /// Access to button map
  void setButtonMap(const std::map<std::string, std::string>& m)
  {
    this->ButtonMapping = m;
    this->ButtonPresent = (this->ButtonMapping.size() > 0);
  }

  /// Access to tracker map
  std::map<std::string, std::string> trackerMap() { return this->TrackerMapping; }
  /// Access to tracker map
  void setTrackerMap(const std::map<std::string, std::string>& m)
  {
    this->TrackerMapping = m;
    this->TrackerPresent = (this->TrackerMapping.size() > 0);
  }

protected slots:
  /// This is called by VRPNEventListener in a threadsafe manner, and should not
  /// be called directly.
  void listen();

protected:
  std::string name(int eventType, int id = 0);

  friend class pqVREventPlayer;
  void newAnalogValue(vrpn_ANALOGCB data);
  void newButtonValue(vrpn_BUTTONCB data);
  void newTrackerValue(vrpn_TRACKERCB data);
  void verifyConfig(const char* id, const char* name);

  void configureTransform(vtkPVXMLElement* child);
  void saveButtonEventConfig(vtkPVXMLElement* child) const;
  void saveAnalogEventConfig(vtkPVXMLElement* child) const;
  void saveTrackerEventConfig(vtkPVXMLElement* child) const;
  void saveTrackerTranslationConfig(vtkPVXMLElement* child) const;
  void saveTrackerRotationConfig(vtkPVXMLElement* child) const;
  void saveTrackerTransformationConfig(vtkPVXMLElement* child) const;
  friend void VRPN_CALLBACK handleAnalogChange(void* userdata, const vrpn_ANALOGCB b);
  friend void VRPN_CALLBACK handleButtonChange(void* userdata, vrpn_BUTTONCB b);
  friend void VRPN_CALLBACK handleTrackerChange(void* userdata, const vrpn_TRACKERCB t);

  std::string Name;
  std::string Address;
  std::string Type;

  // std::map<std::string,std::string> Mapping;
  std::map<std::string, std::string> ButtonMapping;
  std::map<std::string, std::string> AnalogMapping;
  std::map<std::string, std::string> TrackerMapping;

  bool TrackerPresent, ButtonPresent, AnalogPresent, TrackerTransformPresent;
  vtkMatrix4x4* Transformation;

  bool Initialized;

  // The shared thread and listener that listens to incoming events.
  friend class pqVRPNEventListener;
  friend class pqVRPNThreadBridge;
  static pqVRPNEventListener* Listener;

  vtkVRQueue* EventQueue;

  class pqInternals;
  pqInternals* Internals;

private:
  Q_DISABLE_COPY(pqVRPNConnection)
};

#endif
