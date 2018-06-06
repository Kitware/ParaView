/*=========================================================================

   Program: ParaView
   Module:    vtkVRUIConnection.h

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
#ifndef vtkVRUIConnection_h
#define vtkVRUIConnection_h

#include "vtkSmartPointer.h"
#include "vtkVRQueue.h"
#include "vtkVRUITrackerState.h"
#include <QThread>
#include <map>
#include <vector>

class vtkPVXMLElement;
class vtkSMProxyLocator;
class vtkTransform;
class vtkMatrix4x4;

/// Callback to listen to VRUI events
class pqVRUIConnection : public QThread
{
  Q_OBJECT
  typedef QThread Superclass;

public:
  pqVRUIConnection(QObject* parent = 0);
  ~pqVRUIConnection();

  // Description:
  // Address of the device. For example, "Tracker0@localhost"
  void setAddress(std::string address);

  // Description:
  // Address of the device. For example, "Tracker0@localhost"
  std::string address() { return this->Address; }

  /// Port number of the VRUI server. Initial value is 8555.
  void setPort(std::string port);

  /// Port number of the VRUI server. Initial value is 8555.
  std::string port() { return this->Port; }

  /// Set the device name.
  void setName(std::string name);

  // Description:
  // Get the device name.
  std::string name() { return this->Name; }

  /// Add button device
  void addButton(std::string id, std::string name);

  /// Add Analog device
  void addAnalog(std::string id, std::string name);

  /// Add tracking device
  void addTracking(std::string id, std::string name);

  /// Adding a transformation matrix
  void setTransformation(vtkMatrix4x4* matrix);

  /// Initialize the device with the name.
  bool init();

  /// Tell if Init() was called successfully bool GetInitialized() const;

  /// Terminate the thread
  void stop();

  /// Sets the Event Queue into which the vrpn data needs to be written
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
  void run();
  void callback();

protected:
  // void PrintPositionOrientation();
  void getNextPacket();

  std::string name(int eventType, int id = 0);

  void verifyConfig(const char* id, const char* name);

  void getAndEnqueueButtonData();
  void getAndEnqueueAnalogData();
  void getAndEnqueueTrackerData();

  void newAnalogValue(std::vector<float>* data);
  void newButtonValue(int state, int button);
  void newTrackerValue(vtkSmartPointer<vtkVRUITrackerState> data, int sensor);

  void configureTransform(vtkPVXMLElement* child);
  void saveButtonEventConfig(vtkPVXMLElement* child) const;
  void saveAnalogEventConfig(vtkPVXMLElement* child) const;
  void saveTrackerEventConfig(vtkPVXMLElement* child) const;
  void saveTrackerTranslationConfig(vtkPVXMLElement* child) const;
  void saveTrackerRotationConfig(vtkPVXMLElement* child) const;
  void saveTrackerTransformationConfig(vtkPVXMLElement* child) const;

  std::string Name;
  std::string Address;
  std::string Port;
  std::string Type;

  // std::map<std::string,std::string> Mapping;
  std::map<std::string, std::string> ButtonMapping;
  std::map<std::string, std::string> AnalogMapping;
  std::map<std::string, std::string> TrackerMapping;

  bool TrackerPresent, ButtonPresent, AnalogPresent, TrackerTransformPresent;
  vtkMatrix4x4* Transformation;

  bool Initialized;
  bool _Stop;

  vtkVRQueue* EventQueue;
  vtkMatrix4x4* ZUpToYUpMatrix;
  vtkMatrix4x4* Matrix;

  class pqInternals;
  pqInternals* Internals;

private:
  Q_DISABLE_COPY(pqVRUIConnection)
};

#endif
