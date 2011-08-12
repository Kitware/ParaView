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
#ifndef __vtkVRUIConnection_h
#define __vtkVRUIConnection_h

#include <QThread>
#include <vrpn_Tracker.h>
#include <vrpn_Button.h>
#include <vrpn_Analog.h>
#include <vrpn_Dial.h>
#include <vrpn_Text.h>
#include "vtkVRQueue.h"
#include "vtkTransform.h"
#include "vtkMatrix4x4.h"
#include "vtkVRUITrackerState.h"
#include "vtkSmartPointer.h"

#include <map>
#include <vector>

class vtkPVXMLElement;
class vtkSMProxyLocator;

/// Callback to listen to VRPN events
class vtkVRUIConnection : public QThread
{
  Q_OBJECT
  typedef QThread Superclass;
public:
  vtkVRUIConnection(QObject *parent=0);
  ~vtkVRUIConnection();

  // Description:
  // Name of the device. For example, "Tracker0@localhost"
  // Initial value is a NULL pointer.
  void SetAddress(std::string name);

  // Description:
  // Port number of the VRUI server.
  // Initial value is 8555.
  void SetPort(std::string port);

  // Description:
  // Set the device name.
  void SetName(std::string name);

  // Description:
  // Add button device
  void AddButton(std::string id, std::string name);

  // Description:
  // Add Analog device
  void AddAnalog(std::string id,  std::string name );

  // Description:
  // Add tracking device
  void AddTracking( std::string id,  std::string name);

  // Description:
  // Adding a transformation matrix
  void SetTransformation( vtkMatrix4x4* matix );

  // Description:
  // Initialize the device with the name.
  bool Init();

  // Description:
  // Tell if Init() was called succesfully
  // bool GetInitialized() const;

  // Description:
  // Terminate the thread
  void Stop();

  // Description:
  // Sets the Event Queue into which the vrpn data needs to be written
  void SetQueue( vtkVRQueue* queue );

  /// configure the style using the xml configuration.
  virtual bool configure(vtkPVXMLElement* child, vtkSMProxyLocator*);

  /// save the xml configuration.
  virtual vtkPVXMLElement* saveConfiguration() const;

 protected slots:
  void run();
  void callback();

protected:
    // Description:
  void Activate();

  // Description:
  void Deactivate();

  // Description:
  void StartStream();

  // Description:
  void StopStream();

  void PrintPositionOrientation();
  void GetNextPacket();

  std::string GetName( int eventType, int id=0 );

  void verifyConfig( const char* id,
                     const char* name );

  void GetAndEnqueueButtonData();
  void GetAndEnqueueAnalogData();
  void GetAndEnqueueTrackerData();

  void NewAnalogValue(vtkstd::vector<float> *data);
  void NewButtonValue(int state,  int button);
  void NewTrackerValue(vtkSmartPointer<vtkVRUITrackerState> data, int sensor);

  void configureTransform( vtkPVXMLElement* child );
  void saveButtonEventConfig( vtkPVXMLElement* child ) const;
  void saveAnalogEventConfig( vtkPVXMLElement* child ) const;
  void saveTrackerEventConfig( vtkPVXMLElement* child ) const;
  void saveTrackerTranslationConfig( vtkPVXMLElement* child ) const;
  void saveTrackerRotationConfig( vtkPVXMLElement* child ) const;
  void saveTrackerTransformationConfig( vtkPVXMLElement* child ) const;

  std::string Name;
  std::string Address;
  std::string Port;
  std::string Type;

  // std::map<std::string,std::string> Mapping;
  std::map<std::string,std::string> ButtonMapping;
  std::map<std::string,std::string> AnalogMapping;
  std::map<std::string,std::string> TrackerMapping;

  bool TrackerPresent, ButtonPresent, AnalogPresent,  TrackerTransformPresent;
  vtkMatrix4x4 *Transformation;

  bool Initialized;
  bool _Stop;

  vtkVRQueue* EventQueue;

  class pqInternals;
  pqInternals* Internals;

private:
  vtkVRUIConnection(const vtkVRUIConnection&); // Not implemented.
  void operator=(const vtkVRUIConnection&); // Not implemented.
};

#endif
