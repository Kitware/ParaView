/*=========================================================================

  Program:   ParaView
  Module:    vtkTCPNetworkAccessManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTCPNetworkAccessManager
// .SECTION Description
// vtkTCPNetworkAccessManager is a concrete implementation of
// vtkNetworkAccessManager that uses tcp/ip sockets for communication between
// processes. It supports urls that use "tcp" as their protocol specifier.

#ifndef __vtkTCPNetworkAccessManager_h
#define __vtkTCPNetworkAccessManager_h

#include "vtkNetworkAccessManager.h"

class vtkMultiProcessController;

class VTK_EXPORT vtkTCPNetworkAccessManager : public vtkNetworkAccessManager
{
public:
  static vtkTCPNetworkAccessManager* New();
  vtkTypeMacro(vtkTCPNetworkAccessManager, vtkNetworkAccessManager);
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
  //   * tcp://localhost:<port>?listen=true& -- listen for connection on port.
  //   * tcp://localhost:<port>?listen=true&multiple=true -- listen for multiple
  // Examples:
  //   * tcp://medea:12345
  //   * tcp://localhost:12345?listen&handshake=3.8.12
  // Supported parameters:
  // handshake :- specify a message that is matched with the other side
  // listen    :- open a server-socket for a client to connect to
  // multiple  :- leave server-socket open for more than 1 client to connect
  //              (listen must be set to true)
  // nonblocking:- When listen is true, this will result in the call returning
  //               NULL if a client connection is not available immediately.
  //               It leaves the server socket open for client to connect.
  virtual vtkMultiProcessController* NewConnection(const char* url);

  // Description:
  // Used to abort pending connection creation, if any. Refer to
  // NewConnection() for details.
  virtual void AbortPendingConnection();

  // Description:
  // Process any network activity.
  virtual int ProcessEvents(unsigned long timeout_msecs);

  // Description:
  // Returns true is the manager is currently waiting for any connections.
  virtual bool GetPendingConnectionsPresent();

//BTX
protected:
  vtkTCPNetworkAccessManager();
  ~vtkTCPNetworkAccessManager();

  // Description:
  // Connects to remote processes.
  vtkMultiProcessController* ConnectToRemote(const char* hostname, int port,
    const char* handshake);

  // Description:
  // Waits for connection from remote process.
  vtkMultiProcessController* WaitForConnection(int port, bool once,
    const char* handshake, bool nonblocking);

  bool ParaViewHandshake(vtkMultiProcessController* controller,
    bool server_side, const char* handshake);

  bool AbortPendingConnectionFlag;
private:
  vtkTCPNetworkAccessManager(const vtkTCPNetworkAccessManager&); // Not implemented
  void operator=(const vtkTCPNetworkAccessManager&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
