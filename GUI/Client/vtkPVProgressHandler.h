/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProgressHandler.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVProgressHandler - Object to represent the output of a PVSource.
// .SECTION Description
// This object combines methods for accessing parallel VTK data, and also an 
// interface for changing the view of the data.  The interface used to be in a 
// superclass called vtkPVActorComposite.  I want to separate the interface 
// from this object, but a superclass is not the way to do it.

#ifndef __vtkPVProgressHandler_h
#define __vtkPVProgressHandler_h


#include "vtkObject.h"

class vtkPVApplication;
class vtkPVWindow;
class vtkTimerLog;
class vtkMPIController;
class vtkSocketController;
class vtkPVProgressHandlerInternal;

class VTK_EXPORT vtkPVProgressHandler : public vtkObject
{
public:
  static vtkPVProgressHandler* New();
  vtkTypeRevisionMacro(vtkPVProgressHandler, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // Set the application that will drive progress
  virtual void SetPVApplication(vtkPVApplication *pvApp)
    {
    this->Application = pvApp;
    }

  // Description:
  // Invoke the progress event.
  virtual void InvokeProgressEvent(
    vtkPVApplication* pvApp,
    vtkObject* object,
    int val,
    const char* str);

  // Description:
  // This method should be called before removing the object.
  virtual void Cleanup() {}


  // Description:
  // This method resets all the progress counters and prepares progress
  // reporting. All progress events before this call are ignored.
  virtual void PrepareProgress(vtkPVApplication* app);

  // Description:
  // This method collects all outstanding progress messages. All progress
  // events after this call are ignored.
  virtual void CleanupPendingProgress(vtkPVApplication* app);

  // Description:
  // This method register object to be observed.
  virtual void RegisterProgressEvent(vtkObject* po, int id);
  
protected:
  vtkPVProgressHandler();
  ~vtkPVProgressHandler();

  vtkPVApplication* Application;

  int ReceivingProgressReports;

  //BTX
  int ProgressPending;
  int Progress[4];

  // Description:
  // Types of progress handling.
  enum {
    NotSet = 0,
    SingleProcess,
    SingleProcessMPI,
    SatelliteMPI,
    ClientServerClient,
    ClientServerServer,
    ClientServerServerMPI
  };
  //ETX
  
  void DetermineProgressType(vtkPVApplication* app);
  int ProgressType;

  void InvokeSatelliteProgressEvent(vtkPVApplication*, vtkObject*, int val);
  void InvokeRootNodeProgressEvent(vtkPVApplication*, vtkObject*, int val);
  void InvokeRootNodeServerProgressEvent(vtkPVApplication*, vtkObject*, int val);
  int ReceiveProgressFromSatellite(int* id, int* progress);
  void LocalDisplayProgress(vtkPVApplication* app, const char* filter, int progress);
  void HandleProgress(int processid, int filterid, int progress);

  double MinimumProgressInterval;
  vtkTimerLog* ProgressTimer;

  vtkMPIController* MPIController;
  vtkSocketController* SocketController;
  vtkPVProgressHandlerInternal* Internals;

  vtkPVProgressHandler(const vtkPVProgressHandler&); // Not implemented
  void operator=(const vtkPVProgressHandler&); // Not implemented
};

#endif

