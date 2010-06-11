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
// handler is created per connection.
// .SECTION See Also
// vtkPVMPICommunicator

#ifndef __vtkPVProgressHandler_h
#define __vtkPVProgressHandler_h

#include "vtkObject.h"
class vtkProcessModuleConnection;
class vtkMPICommunicatorOpaqueRequest;
class VTK_EXPORT vtkPVProgressHandler : public vtkObject
{
public:
  static vtkPVProgressHandler* New();
  vtkTypeMacro(vtkPVProgressHandler, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the connection. This is not reference-counted to avoid cycles.
  void SetConnection(vtkProcessModuleConnection* conn);
  vtkGetObjectMacro(Connection, vtkProcessModuleConnection);

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
  // Called when the client connection (vtkClientConnection) receives progress
  // from the server.
  void HandleServerProgress(int progress, const char* text);

  // Description:
  // Get/Set the progress frequency in seconds. Default is 0.5 seconds.
  vtkSetClampMacro(ProgressFrequency, double, 0.01, 30.0);
  vtkGetMacro(ProgressFrequency, double);

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

  enum eProcessTypes
    {
    INVALID=0,
    ALL_IN_ONE,
    CLIENTSERVER_CLIENT,
    CLIENTSERVER_SERVER_ROOT,
    SATELLITE
    };
  enum eTAGS
    {
    CLEANUP_TAG = 188969,
    PROGRESS_EVENT_TAG = 188970
    };

  // Description:
  // Determines the process type using the Connection
  void DetermineProcessType();

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

  void SendProgressToClient();
  void SendProgressToRoot();
  int ReceiveProgressFromSatellites();
  void ReceiveProgressFromServer();

  double ProgressFrequency;

  vtkProcessModuleConnection* Connection;
  eProcessTypes ProcessType;
private:
  vtkPVProgressHandler(const vtkPVProgressHandler&); // Not implemented
  void operator=(const vtkPVProgressHandler&); // Not implemented

  // Description:
  // Callback called when vtkCommand::ProgressEvent is received.
  void OnProgressEvent(vtkObject* obj, double progress);

  class vtkInternals;
  vtkInternals* Internals;

  class vtkObserver;
  vtkObserver* Observer;
  friend class vtkObserver;
//ETX
};

#endif

