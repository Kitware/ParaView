/*=========================================================================

  Program:   ParaView
  Module:    vtkNetworkAccessManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkNetworkAccessManager
// .SECTION Description
// vtkNetworkAccessManager is used to create new connections and monitor
// activity of those connections. This is an abstract class that defines the
// interface. Concrete implementations of this class can be written to support
// tcp/ip socket or ssl or ssh based network connections among processes.

#ifndef __vtkNetworkAccessManager_h
#define __vtkNetworkAccessManager_h

#include "vtkObject.h"

class vtkMultiProcessController;

class VTK_EXPORT vtkNetworkAccessManager : public vtkObject
{
public:
  vtkTypeMacro(vtkNetworkAccessManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates a new connection given the url.
  // This call may block until the connection can be established. To keep
  // user-interfaces responsive, one can listen to the vtkCommand::ProgressEvent
  // fired periodically by this class while waiting.
  //
  // vtkNetworkAccessManager can  be waiting for atmost one connection at a
  // time. Calling NewConnection() while another connection is pending will
  // raise an error.
  //
  // To abort the connection and cancel the waiting, simply call
  // AbortPendingConnection() in the vtkCommand::ProgressEvent callback.
  //
  // Returns the new connection instance on success, otherwise NULL.
  //
  // URLs are of the following form:
  // <transport>://<address>
  //   * tcp://<hostname>:<port>
  //   * tcp://localhost:<port>/listen -- listen for connection on port.
  //   * tcp://localhost:<port>/listenmultiple -- listen for multiple
  // Examples:
  //   * tcp://medea:12345
  //   * tcp://localhost:12345/listen
  //   * ssh://utkarsh@medea
  //   * http://kitware-server/session?id=12322&authorization=12
  virtual vtkMultiProcessController* NewConnection(const char* url)=0;

  // Description:
  // Used to abort pending connection creation, if any. Refer to
  // NewConnection() for details.
  virtual void AbortPendingConnection()=0;

  // Description:
  // Process any network activity.
  virtual int ProcessEvents(unsigned long timeout_msecs)=0;

  // Description:
  // Returns true is the manager is currently waiting for any connections.
  virtual bool GetPendingConnectionsPresent()=0;

//BTX
protected:
  vtkNetworkAccessManager();
  ~vtkNetworkAccessManager();

private:
  vtkNetworkAccessManager(const vtkNetworkAccessManager&); // Not implemented
  void operator=(const vtkNetworkAccessManager&); // Not implemented
//ETX
};

#endif
