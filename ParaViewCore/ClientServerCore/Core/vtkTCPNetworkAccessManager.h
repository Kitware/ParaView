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
/**
 * @class   vtkTCPNetworkAccessManager
 *
 * vtkTCPNetworkAccessManager is a concrete implementation of
 * vtkNetworkAccessManager that uses tcp/ip sockets for communication between
 * processes. It supports urls that use "tcp" as their protocol specifier.
*/

#ifndef vtkTCPNetworkAccessManager_h
#define vtkTCPNetworkAccessManager_h

#include "vtkNetworkAccessManager.h"
#include "vtkPVClientServerCoreCoreModule.h" //needed for exports

class vtkMultiProcessController;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkTCPNetworkAccessManager : public vtkNetworkAccessManager
{
public:
  static vtkTCPNetworkAccessManager* New();
  vtkTypeMacro(vtkTCPNetworkAccessManager, vtkNetworkAccessManager);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a new connection given the url.
   * This call may block until the connection can be established. To keep
   * user-interfaces responsive, one can listen to the vtkCommand::ProgressEvent
   * fired periodically by this class while waiting.

   * vtkNetworkAccessManager can  be waiting for atmost one connection at a
   * time. Calling NewConnection() while another connection is pending will
   * raise an error.

   * To abort the connection and cancel the waiting, simply call
   * AbortPendingConnection() in the vtkCommand::ProgressEvent callback.

   * Returns the new connection instance on success, otherwise NULL.

   * URLs are of the following form:
   * \c \<transport\>://\<address\>
   * * \c tcp://<hostname\>:\<port\>
   * * \c tcp://localhost:\<port\>?listen=true& -- listen for connection on port.
   * * \c tcp://localhost:\<port\>?listen=true&multiple=true -- listen for multiple
   * Examples:
   * * \c tcp://medea:12345
   * * \c tcp://localhost:12345?listen&handshake=3.8.12
   * Supported parameters:
   * handshake :- specify a message that is matched with the other side
   * listen    :- open a server-socket for a client to connect to
   * multiple  :- leave server-socket open for more than 1 client to connect
   * (listen must be set to true)
   * nonblocking:- When listen is true, this will result in the call returning
   * NULL if a client connection is not available immediately.
   * It leaves the server socket open for client to connect.
   * timeout   :- When connecting to remote i.e listen==false, specify the time
   * (in seconds) for which this call blocks to retry attempts to
   * connect to the host/port. If absent, default is 60s. 0 implies no retry attempts.
   * A negative value implies an infinite number of retries.
   */
  vtkMultiProcessController* NewConnection(const char* url) override;

  /**
   * Used to abort pending connection creation, if any. Refer to
   * NewConnection() for details.
   */
  void AbortPendingConnection() override;

  /**
   * Process any network activity.
   */
  int ProcessEvents(unsigned long timeout_msecs) override;

  /**
   * Peeks to check if any activity is available. When this call returns true,
   * ProcessEvents() will always result in some activity processing if called
   * afterword.
   */
  bool GetNetworkEventsAvailable() override;

  /**
   * Returns true is the manager is currently waiting for any connections.
   */
  bool GetPendingConnectionsPresent() override;

  /**
   * Enable/disable further connections for given port.
   */
  virtual void DisableFurtherConnections(int port, bool disable) override;

  /**
   * Returns true if the last check of connect ids was wrong.
   */
  virtual bool GetWrongConnectID() override;

protected:
  vtkTCPNetworkAccessManager();
  ~vtkTCPNetworkAccessManager() override;

  // used by GetPendingConnectionsPresent and ProcessEvents
  int ProcessEventsInternal(unsigned long timeout_msecs, bool do_processing);

  /**
   * Connects to remote processes.
   */
  vtkMultiProcessController* ConnectToRemote(
    const char* hostname, int port, const char* handshake, int timeout_in_seconds);

  /**
   * Waits for connection from remote process.
   */
  vtkMultiProcessController* WaitForConnection(
    int port, bool once, const char* handshake, bool nonblocking);

  enum HandshakeErrors
  {
    HANDSHAKE_NO_ERROR = 0,
    HANDSHAKE_SOCKET_COMMUNICATOR_DIFFERENT,
    HANDSHAKE_DIFFERENT_PV_VERSIONS,
    HANDSHAKE_DIFFERENT_CONNECTION_IDS,
    HANDSHAKE_DIFFERENT_RENDERING_BACKENDS,
    HANDSHAKE_UNKNOWN_ERROR
  };

  int ParaViewHandshake(
    vtkMultiProcessController* controller, bool server_side, const char* handshake);
  void PrintHandshakeError(int errorcode, bool server_side);
  int AnalyzeHandshakeAndGetErrorCode(const char* clientHS, const char* serverHS);

  bool AbortPendingConnectionFlag;
  bool WrongConnectID;

private:
  vtkTCPNetworkAccessManager(const vtkTCPNetworkAccessManager&) = delete;
  void operator=(const vtkTCPNetworkAccessManager&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
