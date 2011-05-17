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
// .NAME vtkSMCollaborationManager - Class used to broadcast message from
// one client to the others.
// .SECTION Description
// This class allow to trigger protobuf messages on all the clients that are
// connected to the server. Those clients can attach listeners and
// handle those message in the way they want.
// The message sender do not receive its message again, only other clients do.
//
// To listen collaboration notification messages you should have a code
// that look like that:
//
// collaborationManager->AddObserver(
//          vtkSMCollaborationManager::CollaborationNotification,
//          callback);
//
// void callback(vtkObject* src, unsigned long event, void* method, void* data)
// {
//   vtkSMMessage* msg = reinterpret_cast<vtkSMMessage*>(data);
//   => do what you want with the message
// }

#ifndef __vtkSMCollaborationManager_h
#define __vtkSMCollaborationManager_h

#include "vtkSMRemoteObject.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage

class vtkSMProxyLocator;
class vtkPVMultiClientsInformation;

class VTK_EXPORT vtkSMCollaborationManager : public vtkSMRemoteObject
{
public:
  // Description:
  // Return the GlobalID that should be used to refer to the TimeKeeper
  static vtkTypeUInt32 GetReservedGlobalID();

  static vtkSMCollaborationManager* New();
  vtkTypeMacro(vtkSMCollaborationManager,vtkSMRemoteObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the global unique id for this object. If none is set and the session is
  // valid, a new global id will be assigned automatically.
  virtual vtkTypeUInt32 GetGlobalID();

//BTX
  enum EventType
    {
    CollaborationNotification = 12345
    };

  // Description:
  // Send message to other clients which will trigger Observer
  void SendToOtherClients(vtkSMMessage* msg);

  // Description:
  // This method return NULL because that class have no state.
  virtual const vtkSMMessage* GetFullState() { return NULL; };

  // Description:
  // This method is used IN THAT SPECIAL CASE to notify external listeners
  virtual void LoadState( const vtkSMMessage* msg, vtkSMProxyLocator* locator);

  // Description:
  // This method is used promote a new Master user. Master/Slave user doesn't
  // buy you anything here. It just provide you the information, and it is your
  // call to prevent slaves users to do or achieve some actions inside your client.
  // When you call that method a SMMessage is also propagated to the other client
  // so they could follow who is the Master without fetching the information again.
  virtual void PromoteToMaster(int clientId);

  // Description:
  // Based on the chached information, it tells you if you are a master or not
  virtual bool IsMaster();

  // Description:
  // Based on the chached information, it tells who's the master
  virtual int GetMasterId();

  // Description:
  // Update the cache for the master informations
  virtual void UpdateMasterInformation();

protected:
  // Description:
  // Default constructor.
  vtkSMCollaborationManager();

  // Description:
  // Destructor.
  virtual ~vtkSMCollaborationManager();

  vtkPVMultiClientsInformation* InformationOnMasterUser;

private:
  vtkSMCollaborationManager(const vtkSMCollaborationManager&); // Not implemented
  void operator=(const vtkSMCollaborationManager&);       // Not implemented
//ETX
};
#endif // #ifndef __vtkSMCollaborationManager_h
