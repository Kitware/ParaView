/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSessionServer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVSessionServer
 *
 * vtkSMSessionServer is a session used on data and/or render servers. It's
 * designed for a process that works with a separate client process that acts as
 * the visualization driver.
 * @sa
 * vtkSMSessionClient
*/

#ifndef vtkPVSessionServer_h
#define vtkPVSessionServer_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkPVSessionBase.h"

class vtkMultiProcessController;
class vtkMultiProcessStream;

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkPVSessionServer : public vtkPVSessionBase
{
public:
  static vtkPVSessionServer* New();
  vtkTypeMacro(vtkPVSessionServer, vtkPVSessionBase);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Returns the active controller used to communicate with the process.
   * Value must be DATA_SERVER_ROOT or RENDER_SERVER_ROOT or CLIENT.
   * But only the CLIENT do return something different than NULL;
   */
  virtual vtkMultiProcessController* GetController(ServerFlags processType) VTK_OVERRIDE;

  /**
   * Connects a remote server. URL can be of the following format:
   * cs://<pvserver-host>:<pvserver-port>
   * cdsrs://<pvdataserver-host>:<pvdataserver-port>/<pvrenderserver-host>:<pvrenderserver-port>
   * In both cases the port is optional. When not provided default
   * pvserver/pvdataserver port // is 11111, while default pvrenderserver port
   * is 22221.
   * For reverse connect i.e. the client waits for the server to connect back,
   * simply add "rc" to the protocol e.g.
   * csrc://<pvserver-host>:<pvserver-port>
   * cdsrsrc://<pvdataserver-host>:<pvdataserver-port>/<pvrenderserver-host>:<pvrenderserver-port>
   * In this case, the hostname is irrelevant and is ignored.
   */
  virtual bool Connect(const char* url);

  /**
   * Overload that constructs the url using the command line parameters
   * specified and then calls Connect(url).
   */
  bool Connect();

  /**
   * Returns true is this session is active/alive/valid.
   */
  virtual bool GetIsAlive() VTK_OVERRIDE;

  /**
   * Client-Server Communication tags.
   */
  enum
  {
    PUSH = 12,
    PULL = 13,
    EXECUTE_STREAM = 14,
    GATHER_INFORMATION = 15,
    REGISTER_SI = 16,
    UNREGISTER_SI = 17,
    LAST_RESULT = 18,
    SERVER_NOTIFICATION_MESSAGE_RMI = 55624,
    CLIENT_SERVER_MESSAGE_RMI = 55625,
    CLOSE_SESSION = 55626,
    REPLY_GATHER_INFORMATION_TAG = 55627,
    REPLY_PULL = 55628,
    REPLY_LAST_RESULT = 55629,
    EXECUTE_STREAM_TAG = 55630
  };

  //@{
  /**
   * Enable or Disable multi-connection support.
   * The MultipleConnection is only used inside the DATA_SERVER to support
   * several clients to connect to it.
   * By default we allow collaboration (this->MultipleConnection = true)
   */
  vtkBooleanMacro(MultipleConnection, bool);
  vtkSetMacro(MultipleConnection, bool);
  vtkGetMacro(MultipleConnection, bool);
  //@}

  void OnClientServerMessageRMI(void* message, int message_length);
  void OnCloseSessionRMI();

  /**
   * Sends the message to all clients.
   */
  virtual void NotifyAllClients(const vtkSMMessage*) VTK_OVERRIDE;

  /**
   * Sends the message to all but the active client-session.
   */
  virtual void NotifyOtherClients(const vtkSMMessage*) VTK_OVERRIDE;

protected:
  vtkPVSessionServer();
  ~vtkPVSessionServer();

  /**
   * Called when client triggers GatherInformation().
   */
  void GatherInformationInternal(
    vtkTypeUInt32 location, const char* classname, vtkTypeUInt32 globalid, vtkMultiProcessStream&);

  /**
   * Sends the last result to client.
   */
  void SendLastResultToClient();

  vtkMPIMToNSocketConnection* MPIMToNSocketConnection;

  bool MultipleConnection;

  class vtkInternals;
  vtkInternals* Internal;
  friend class vtkInternals;

private:
  vtkPVSessionServer(const vtkPVSessionServer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVSessionServer&) VTK_DELETE_FUNCTION;
};

#endif
