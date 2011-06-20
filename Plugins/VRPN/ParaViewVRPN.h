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
#ifndef __ParaViewVRPN_h
#define __ParaViewVRPN_h

#include <QThread>
#include <vrpn_Tracker.h>
#include <vrpn_Button.h>
#include <vrpn_Analog.h>
#include <vrpn_Dial.h>
#include <vrpn_Text.h>
#include "vtkVRQueue.h"


typedef QThread vtkThread;

/// Callback to listen to VRPN events
class ParaViewVRPN : public vtkThread
{
  Q_OBJECT
public:
  ParaViewVRPN();
  ~ParaViewVRPN();

  // Description:
  // Name of the device. For example, "Tracker0@localhost"
  // Initial value is a NULL pointer.
  void SetName(std::string name);

  // Description:
  // Initialize the device with the name.
  void Init();

  // Description:
  // Tell if Init() was called succesfully
  bool GetInitialized() const;

  // Description:
  // Terminate the thread
  void terminate();

  // Description:
  // Sets the Event Queue into which the vrpn data needs to be written
  void SetQueue( vtkVRQueue* queue );

 protected slots:
  void run();


protected:
  void NewAnalogValue(vrpn_ANALOGCB data);
  void NewButtonValue(vrpn_BUTTONCB data);
  void NewTrackerValue(vrpn_TRACKERCB data );

  friend void VRPN_CALLBACK handleAnalogChange(void* userdata, const vrpn_ANALOGCB b);
  friend void VRPN_CALLBACK handleButtonChange(void* userdata, vrpn_BUTTONCB b);
  friend void VRPN_CALLBACK handleTrackerChange(void *userdata, const vrpn_TRACKERCB t);

  std::string Name;
  bool Initialized;
  bool _Stop;

  vtkVRQueue* EventQueue;

  class pqInternals;
  pqInternals* Internals;


private:
  ParaViewVRPN(const ParaViewVRPN&); // Not implemented.
  void operator=(const ParaViewVRPN&); // Not implemented.
};

#endif
