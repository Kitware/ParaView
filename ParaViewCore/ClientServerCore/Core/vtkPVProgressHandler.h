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
// .NAME vtkPVProgressHandler - progress handler.
// .SECTION Description
// vtkPVProgressHandler handles the progress messages. It handles progress in
// all configurations single process, client-server, mpi-batch. One progress
// handler is created per Session.
//
// Progress events are currently not supported in multi-clients mode.
//
// .SECTION Events
// vtkCommand::StartEvent
// \li fired to indicate beginning of progress handling
// \li \c calldata: vtkPVProgressHandler*
// vtkCommand::ProgressEvent
// \li fired to indicate a progress event.
// \li \c calldata: vtkPVProgressHandler*
// vtkCommand::EndEvent
// \li fired to indicate end of progress handling
// \li \c calldata: vtkPVProgressHandler*
// .SECTION See Also
// vtkPVMPICommunicator

#ifndef __vtkPVProgressHandler_h
#define __vtkPVProgressHandler_h

#include "vtkObject.h"

class vtkMPICommunicatorOpaqueRequest;
class vtkMultiProcessController;
class vtkPVSession;

class VTK_EXPORT vtkPVProgressHandler : public vtkObject
{
public:
  static vtkPVProgressHandler* New();
  vtkTypeMacro(vtkPVProgressHandler, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the session. This is not reference-counted to avoid cycles.
  void SetSession(vtkPVSession* conn);
  vtkGetObjectMacro(Session, vtkPVSession);

  // Description:
  // Listen to progress events from the object.
  void RegisterProgressEvent(vtkObject* object, int id);

  // Description:
  // This method resets all the progress counters and prepares progress
  // reporting. All progress events before this call are ignored.
  void PrepareProgress();

  // Description:
  // This method collects all outstanding progress messages. All progress
  // events after this call are ignored.
  void CleanupPendingProgress();

  // Description:
  // Get/Set the progress frequency in seconds. Default is 0.5 seconds.
  vtkSetClampMacro(ProgressFrequency, double, 0.01, 30.0);
  vtkGetMacro(ProgressFrequency, double);

  // Description:
  // These are only valid in handler for the vtkCommand::ProgressEvent.
  vtkGetStringMacro(LastProgressText);
  vtkGetMacro(LastProgress, int);

//BTX
  // Description:
  // These methods are used by vtkPVMPICommunicator to handle the progress
  // messages received from satellites while waiting on some receive.
  vtkMPICommunicatorOpaqueRequest* GetAsyncRequest();
  void RefreshProgress();
  void MarkAsyncRequestReceived();
protected:
  vtkPVProgressHandler();
  ~vtkPVProgressHandler();

  enum eTAGS
    {
    CLEANUP_TAG = 188969,
    PROGRESS_EVENT_TAG = 188970
    };

  // Description:
  // Called when the client connection (vtkClientConnection) receives progress
  // from the server.
  void HandleServerProgress(int progress, const char* text);

  // Description:
  // Called on MPI processes to pass around the CLEANUP_TAG marking the end of
  // the progress messages.
  void CleanupSatellites();

  // Description:
  // Returns if current process is the root node.
  bool GetIsRoot();

  // Description:
  // Called on a MPI group to gather progress.
  int GatherProgress();

  // Description:
  // Returns if the progress should be reported.
  bool ReportProgress(double progress);

  void SetLocalProgress(int progress, const char* text);
  void SendProgressToClient(vtkMultiProcessController*);
  void SendProgressToRoot();
  int ReceiveProgressFromSatellites();
  void ReceiveProgressFromServer(vtkMultiProcessController*);

  vtkSetStringMacro(LastProgressText);
  int LastProgress;
  char* LastProgressText;
  double ProgressFrequency;
  vtkPVSession* Session;
private:
  vtkPVProgressHandler(const vtkPVProgressHandler&); // Not implemented
  void operator=(const vtkPVProgressHandler&); // Not implemented

  // Description:
  // Callback called when vtkCommand::ProgressEvent is received.
  void OnProgressEvent(vtkObject* obj, double progress);

  // Description:
  // Callback called when WrongTagEvent is fired by the controllers.
  bool OnWrongTagEvent(void* calldata);

  bool AddedHandlers;
  class vtkInternals;
  vtkInternals* Internals;

  class vtkObserver;
  vtkObserver* Observer;
  friend class vtkObserver;
//ETX
};

#endif

