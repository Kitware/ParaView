// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkNetworkAccessManager
 *
 * vtkNetworkAccessManager is used to create new connections and monitor
 * activity of those connections. This is an abstract class that defines the
 * interface. Concrete implementations of this class can be written to support
 * tcp/ip socket or ssl or ssh based network connections among processes.
 */

#ifndef vtkNetworkAccessManager_h
#define vtkNetworkAccessManager_h

#include "vtkObject.h"
#include "vtkRemotingCoreModule.h" //needed for exports

class vtkMultiProcessController;

class VTKREMOTINGCORE_EXPORT vtkNetworkAccessManager : public vtkObject
{
public:
  vtkTypeMacro(vtkNetworkAccessManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Possible result of connection when creating a new connection
   * CONNECTION_SUCCESS: Connection was sucessfull
   * CONNECTION_TIMEOUT: After spending a specified amount of time, no connection was initiated
   * CONNECTION_ABORT: User aborted the connection before it was successful or timed out
   * CONNECTION_HANDSHAKE_ERROR: Connection was iniated but the handshake failed
   * CONNECTION_FAILURE: Unspecified connection error
   */
  enum class ConnectionResult
  {
    CONNECTION_SUCCESS,
    CONNECTION_TIMEOUT,
    CONNECTION_ABORT,
    CONNECTION_HANDSHAKE_ERROR,
    CONNECTION_FAILURE
  };

  ///@{
  /**
   * Creates a new connection given the url.
   * This call may block until the connection can be established. To keep
   * user-interfaces responsive, one can listen to the vtkCommand::ProgressEvent
   * fired periodically by this class while waiting.
   * The result arg provide information about the failure or sucess of the connection,
   * see vtkNetworkAccessManager::ConnectionResult for possible values.

   * vtkNetworkAccessManager can  be waiting for atmost one connection at a
   * time. Calling NewConnection() while another connection is pending will
   * raise an error.

   * To abort the connection and cancel the waiting, simply call
   * AbortPendingConnection() in the vtkCommand::ProgressEvent callback.

   * Returns the new connection instance on success, otherwise nullptr.

   * URLs are of the following form:
   * \p \<transport\>://\<address\>
   * * \p tcp://\<hostname\>:\<port\>
   * * \p tcp://localhost:\<port\>/listen -- listen for connection on port.
   * * \p tcp://localhost:\<port\>/listenmultiple -- listen for multiple
   * Examples:
   * * \p tcp://medea:12345
   * * \p tcp://localhost:12345/listen
   * * \p ssh://utkarsh\@medea
   * * http://kitware-server/session?id=12322&authorization=12
   */
  virtual vtkMultiProcessController* NewConnection(const char* url)
  {
    ConnectionResult result;
    return this->NewConnection(url, result);
  }
  virtual vtkMultiProcessController* NewConnection(const char* url, ConnectionResult& result) = 0;
  ///@}

  /**
   * Used to abort pending connection creation, if any. Refer to
   * NewConnection() for details.
   */
  virtual void AbortPendingConnection() = 0;

  /**
   * Process any network activity.
   */
  virtual int ProcessEvents(unsigned long timeout_msecs) = 0;

  /**
   * Peeks to check if any activity is available. When this call returns true,
   * ProcessEvents() will always result in some activity processing if called
   * afterword.
   */
  virtual bool GetNetworkEventsAvailable() = 0;

  /**
   * Returns true is the manager is currently waiting for any connections.
   */
  virtual bool GetPendingConnectionsPresent() = 0;

  /**
   * Enable/disable further connections for given port.
   */
  virtual void DisableFurtherConnections(int port, bool disable) = 0;

  /**
   * Returns true if the last check of connect ids was wrong.
   */
  virtual bool GetWrongConnectID() = 0;

protected:
  vtkNetworkAccessManager();
  ~vtkNetworkAccessManager() override;

private:
  vtkNetworkAccessManager(const vtkNetworkAccessManager&) = delete;
  void operator=(const vtkNetworkAccessManager&) = delete;
};

#endif
