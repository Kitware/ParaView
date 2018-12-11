/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCollaborationManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMCollaborationManager
 * @brief   Class used to broadcast message from
 * one client to the others.
 *
 * This class allow to trigger protobuf messages on all the clients that are
 * connected to the server. Those clients can attach listeners and
 * handle those message in the way they want.
 * The message sender do not receive its message again, only other clients do.
 *
 * To listen collaboration notification messages you should have a code
 * that look like that:
 *
 * collaborationManager->AddObserver(
 *          vtkSMCollaborationManager::CollaborationNotification,
 *          callback);
 *
 * void callback(vtkObject* src, unsigned long event, void* method, void* data)
 * {
 *   vtkSMMessage* msg = reinterpret_cast<vtkSMMessage*>(data);
 *   => do what you want with the message
 * }
*/

#ifndef vtkSMCollaborationManager_h
#define vtkSMCollaborationManager_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMMessageMinimal.h"          // needed for vtkSMMessage
#include "vtkSMRemoteObject.h"

class vtkSMProxyLocator;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMCollaborationManager : public vtkSMRemoteObject
{
public:
  /**
   * Return the GlobalID that should be used to refer to the TimeKeeper
   */
  static vtkTypeUInt32 GetReservedGlobalID();

  static vtkSMCollaborationManager* New();
  vtkTypeMacro(vtkSMCollaborationManager, vtkSMRemoteObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the global unique id for this object. If none is set and the session is
   * valid, a new global id will be assigned automatically.
   */
  vtkTypeUInt32 GetGlobalID() override;

  /**
   * Override the session setting in order to update only once our current
   * local user id
   */
  void SetSession(vtkSMSession*) override;

  /**
   * This method is used promote a new Master user. Master/Slave user doesn't
   * buy you anything here. It just provide you the information, and it is your
   * call to prevent slaves users to do or achieve some actions inside your client.
   * When you call that method a SMMessage is also propagated to the other client
   * so they could follow who is the Master without fetching the information again.
   */
  virtual void PromoteToMaster(int clientId);

  /**
   * Share the decision that user should follow that given user if master or
   * follow someone else on your own
   */
  virtual void FollowUser(int clientId);

  /**
   * Return the local followed user
   */
  int GetFollowedUser();

  /**
   * Return true if the current client is the master
   */
  virtual bool IsMaster();

  /**
   * Return the userId of the current master
   */
  virtual int GetMasterId();

  /**
   * Return true if further connections are disabled.
   */
  bool GetDisableFurtherConnections();

  /**
   * Return the id of the current client
   */
  virtual int GetUserId();

  /**
   * Return the id of the nth connected client.
   * In the list you will find yourself as well.
   */
  virtual int GetUserId(int index);

  /**
   * return the name of the provided userId
   */
  virtual const char* GetUserLabel(int userID);

  /**
   * Update ou local user name
   */
  virtual void SetUserLabel(const char* userName);

  /**
   * Update any user name
   */
  virtual void SetUserLabel(int userId, const char* userName);

  /**
   * return the number of currently connected clients. This size is used to bound
   * the GetUserId() method.
   */
  virtual int GetNumberOfConnectedClients();

  /**
   * Request an update of the user list from the server. (A pull request is done)
   */
  void UpdateUserInformations();

  /**
   * Return the server connect id if this is the master.
   * Else return -1.
   */
  int GetServerConnectID();

  /**
   * Return the client connect id.
   */
  int GetConnectID();

  enum EventType
  {
    CollaborationNotification = 12345,
    UpdateUserName = 12346,
    UpdateUserList = 12347,
    UpdateMasterUser = 12348,
    FollowUserCamera = 12349,
    CameraChanged = 12350
  };

  /**
   * Send message to other clients which will trigger Observer
   */
  void SendToOtherClients(vtkSMMessage* msg);

  /**
   * This method return the state of the connected clients
   */
  const vtkSMMessage* GetFullState() override;

  /**
   * This method is used to either load its internal connected clients
   * information or to forward messages across clients
   */
  void LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator) override;

  /**
   * Enable or disable further connections to the server.
   * Already connected clients stay connected.
   */
  void DisableFurtherConnections(bool disable);

  /**
   * Change the connect-id. Already connected clients stay connected.
   * @param connectID the new connect-id for the server.
   */
  void SetConnectID(int connectID);

protected:
  /**
   * Default constructor.
   */
  vtkSMCollaborationManager();

  /**
   * Destructor.
   */
  ~vtkSMCollaborationManager() override;

private:
  class vtkInternal;
  vtkInternal* Internal;

  vtkSMCollaborationManager(const vtkSMCollaborationManager&) = delete;
  void operator=(const vtkSMCollaborationManager&) = delete;
};
#endif // #ifndef vtkSMCollaborationManager_h
