/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSession.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVSession
 * @brief   extends vtkSession to add API for ParaView sessions.
 *
 * vtkPVSession adds APIs to vtkSession for ParaView-specific sessions, namely
 * those that are used to communicate between data-server,render-server and
 * client. This is an abstract class.
*/

#ifndef vtkPVSession_h
#define vtkPVSession_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkSession.h"

class vtkMPIMToNSocketConnection;
class vtkMultiProcessController;
class vtkPVProgressHandler;
class vtkPVServerInformation;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVSession : public vtkSession
{
public:
  vtkTypeMacro(vtkPVSession, vtkSession);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum ServerFlags
  {
    NONE = 0,
    DATA_SERVER = 0x01,
    DATA_SERVER_ROOT = 0x02,
    RENDER_SERVER = 0x04,
    RENDER_SERVER_ROOT = 0x08,
    SERVERS = DATA_SERVER | RENDER_SERVER,
    CLIENT = 0x10,
    CLIENT_AND_SERVERS = DATA_SERVER | CLIENT | RENDER_SERVER
  };

  /**
   * Returns a ServerFlags indicate the nature of the current processes. e.g. if
   * the current processes acts as a data-server and a render-server, it returns
   * DATA_SERVER | RENDER_SERVER.
   */
  virtual ServerFlags GetProcessRoles();

  /**
   * Convenience method that returns true if the current session is serving the
   * indicated role on this process.
   */
  bool HasProcessRole(vtkTypeUInt32 flag)
  {
    return ((flag & static_cast<vtkTypeUInt32>(this->GetProcessRoles())) == flag);
  }

  /**
   * Returns the controller used to communicate with the process. Value must be
   * DATA_SERVER_ROOT or RENDER_SERVER_ROOT or CLIENT.
   * Default implementation returns NULL.
   */
  virtual vtkMultiProcessController* GetController(ServerFlags processType);

  /**
   * This is socket connection, if any to communicate between the data-server
   * and render-server nodes.
   */
  virtual vtkMPIMToNSocketConnection* GetMPIMToNSocketConnection() { return NULL; }

  /**
   * vtkPVServerInformation is an information-object that provides information
   * about the server processes. These include server-side capabilities as well
   * as server-side command line arguments e.g. tile-display parameters. Use
   * this method to obtain the server-side information.
   * NOTE: For now, we are not bothering to provide separate information from
   * data-server and render-server (as was the case earlier). We can easily add
   * API for the same if needed.
   */
  virtual vtkPVServerInformation* GetServerInformation() = 0;

  /**
   * Allow anyone to know easily if the current session is involved in
   * collaboration or not. This is mostly true for the Client side.
   */
  virtual bool IsMultiClients();

  //@{
  /**
   * Provides access to the progress handler.
   */
  vtkGetObjectMacro(ProgressHandler, vtkPVProgressHandler);
  //@}

  //@{
  /**
   * Should be called to begin/end receiving progresses on this session.
   */
  void PrepareProgress();
  void CleanupPendingProgress();
  //@}

  /**
   * Returns true if the session is within a PrepareProgress() and
   * CleanupPendingProgress() block.
   */
  bool GetPendingProgress();

protected:
  vtkPVSession();
  ~vtkPVSession() override;

  enum
  {
    EXCEPTION_EVENT_TAG = 31416
  };

  /**
   * Callback when any vtkMultiProcessController subclass fires a WrongTagEvent.
   * Return true if the event was handle localy.
   */
  virtual bool OnWrongTagEvent(vtkObject* caller, unsigned long eventid, void* calldata);

  //@{
  /**
   * Virtual methods subclasses can override.
   */
  virtual void PrepareProgressInternal();
  virtual void CleanupPendingProgressInternal();
  //@}

  vtkPVProgressHandler* ProgressHandler;

private:
  vtkPVSession(const vtkPVSession&) = delete;
  void operator=(const vtkPVSession&) = delete;

  int ProgressCount;
  // This flags ensures that while we are waiting for an previous progress-pair
  // to finish, we don't start new progress-pairs.
  bool InCleanupPendingProgress;
};

#endif
