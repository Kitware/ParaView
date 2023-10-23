// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqVRPNConnection_h
#define pqVRPNConnection_h

#include <QtCore/QObject>

#include "vtkMatrix4x4.h"
#include "vtkTransform.h"

#include <QtCore/QMutex>

#include <vrpn_Analog.h>
#include <vrpn_Button.h>
#include <vrpn_Tracker.h>

#include <map>

class pqVRPNEventListener;
class vtkVRQueue;

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
  // Add Valuator device
  void addValuator(std::string id, std::string name);

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

  /// Access to valuator map
  std::map<std::string, std::string> valuatorMap() { return this->ValuatorMapping; }
  /// Access to valuator map
  void setValuatorMap(const std::map<std::string, std::string>& mapping)
  {
    this->ValuatorMapping = mapping;
    this->ValuatorPresent = (this->ValuatorMapping.size() > 0);
  }

  /// Access to button map
  std::map<std::string, std::string> buttonMap() { return this->ButtonMapping; }
  /// Access to button map
  void setButtonMap(const std::map<std::string, std::string>& mapping)
  {
    this->ButtonMapping = mapping;
    this->ButtonPresent = (this->ButtonMapping.size() > 0);
  }

  /// Access to tracker map
  std::map<std::string, std::string> trackerMap() { return this->TrackerMapping; }
  /// Access to tracker map
  void setTrackerMap(const std::map<std::string, std::string>& mapping)
  {
    this->TrackerMapping = mapping;
    this->TrackerPresent = (this->TrackerMapping.size() > 0);
  }

protected Q_SLOTS:
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
  void saveValuatorEventConfig(vtkPVXMLElement* child) const;
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
  std::map<std::string, std::string> ValuatorMapping;
  std::map<std::string, std::string> TrackerMapping;

  bool TrackerPresent, ButtonPresent, ValuatorPresent, TrackerTransformPresent;
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
