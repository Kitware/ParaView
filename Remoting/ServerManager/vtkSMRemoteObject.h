/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRemoteObject.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMRemoteObject
 * @brief   baseclass for all proxy-objects that have counter
 * parts on server as well as client processes.
 *
 * Abstract class involved in ServerManager class hierarchy that has a
 * corresponding SIObject which can be local or remote.
*/

#ifndef vtkSMRemoteObject_h
#define vtkSMRemoteObject_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMMessageMinimal.h"            // needed for vtkSMMessage
#include "vtkSMSessionObject.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer

class vtkClientServerStream;
class vtkSMSession;
class vtkSMProxyLocator;
class vtkSMLoadStateContext;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMRemoteObject : public vtkSMSessionObject
{
  // My friends are...
  friend class vtkSMStateHelper; // To pull state
  friend class vtkSMStateLoader; // To set GlobalId as the originals

public:
  vtkTypeMacro(vtkSMRemoteObject, vtkSMSessionObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the location where the underlying VTK-objects are created. The
   * value can be constructed by or-ing vtkSMSession::ServerFlags
   */
  vtkSetMacro(Location, vtkTypeUInt32);
  vtkGetMacro(Location, vtkTypeUInt32);
  //@}

  /**
   * Override the SetSession so if the object already have an ID
   * we automatically register it to the associated session
   */
  void SetSession(vtkSMSession*) override;

  //@{
  /**
   * Get the global unique id for this object. If none is set and the session is
   * valid, a new global id will be assigned automatically.
   */
  virtual vtkTypeUInt32 GetGlobalID();
  const char* GetGlobalIDAsString();
  //@}

  /**
   * Allow the user to test if the RemoteObject has already a GlobalID without
   * assigning a new one to it.
   */
  bool HasGlobalID();

  //@{
  /**
   * Allow user to set the remote object to be discard for Undo/Redo
   * action. By default, any remote object is Undoable.
   */
  vtkBooleanMacro(Prototype, bool);
  bool IsPrototype() { return this->Prototype; }
  vtkSetMacro(Prototype, bool);
  //@}

  /**
   * This method return the full object state that can be used to create that
   * object from scratch.
   * This method will be used to fill the undo stack.
   * If not overridden this will return nullptr.
   */
  virtual const vtkSMMessage* GetFullState() { return nullptr; }

  //@{
  /**
   * This method is used to initialise the object to the given state
   * If the definitionOnly Flag is set to True the proxy won't load the
   * properties values and just setup the new proxy hierarchy with all subproxy
   * globalID set. This allow to split the load process in 2 step to prevent
   * invalid state when property refere to a sub-proxy that does not exist yet.
   */
  virtual void LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator)
  {
    (void)msg;
    (void)locator;
  }
  //@}

  /**
   * Allow to switch off any push of state change to the server for that
   * particular object.
   * This is used when we load a state based on a server notification. In that
   * particular case, the server is already aware of that new state, so we keep
   * those changes local.
   */
  virtual void EnableLocalPushOnly();

  /**
   * Enable the given remote object to communicate its state normally to the
   * server location.
   */
  virtual void DisableLocalPushOnly();

  /**
   * Let the session be aware that even if the Location is client only,
   * the message should not be send to the server for a general broadcast
   */
  virtual bool IsLocalPushOnly() { return this->ClientOnlyLocationFlag; }

protected:
  /**
   * Default constructor.
   */
  vtkSMRemoteObject();

  /**
   * Destructor.
   */
  ~vtkSMRemoteObject() override;

  /**
   * Subclasses can call this method to send a message to its state
   * object on  the server processes specified.
   */
  void PushState(vtkSMMessage* msg);

  /**
   * Subclasses can call this method to pull the state from the
   * state-object on the server processes specified. Returns true on successful
   * fetch. The message is updated with the fetched state.
   */
  bool PullState(vtkSMMessage* msg);

  /**
   * Set the GlobalUniqueId
   */
  void SetGlobalID(vtkTypeUInt32 guid);

  // Global-ID for this vtkSMRemoteObject. This is assigned when needed.
  // Assigned at :
  // - First push
  // - or when the RemoteObject is created by the ProcessModule remotely.
  // - or when state is loaded from protobuf messages
  vtkTypeUInt32 GlobalID;

  // Location flag identify the processes on which the vtkSIObject
  // corresponding to this vtkSMRemoteObject exist.
  vtkTypeUInt32 Location;

  // Allow remote object to be discard for any state management such as
  // Undo/Redo, Register/UnRegister (in ProxyManager) and so on...
  bool Prototype;

  // Field that store the Disable/EnableLocalPushOnly() state information
  bool ClientOnlyLocationFlag;

  // Convenient method used to return either the local Location or a filtered
  // version of it based on the ClientOnlyLocationFlag
  vtkTypeUInt32 GetFilteredLocation();

private:
  vtkSMRemoteObject(const vtkSMRemoteObject&) = delete;
  void operator=(const vtkSMRemoteObject&) = delete;

  char* GlobalIDString;
};

/// This defines a manipulator for the vtkClientServerStream that can be used
/// to indicate to the interpreter that the placeholder is to be replaced by
/// the vtkSIProxy instance for the given vtkSMProxy instance.
/// e.g.
/// \code
/// vtkClientServerStream stream;
/// stream << vtkClientServerStream::Invoke
///        << SIOBJECT(proxyA)
///        << "MethodName"
///        << vtkClientServerStream::End;
/// \endcode
/// Will result in calling the vtkSIProxy::MethodName() when the stream in
/// interpreted.
class VTKREMOTINGSERVERMANAGER_EXPORT SIOBJECT
{
  vtkSMRemoteObject* Reference;
  friend VTKREMOTINGSERVERMANAGER_EXPORT vtkClientServerStream& operator<<(
    vtkClientServerStream& stream, const SIOBJECT& manipulator);

public:
  SIOBJECT(vtkSMRemoteObject* rmobject)
    : Reference(rmobject)
  {
  }
};

VTKREMOTINGSERVERMANAGER_EXPORT vtkClientServerStream& operator<<(
  vtkClientServerStream& stream, const SIOBJECT& manipulator);

#endif // #ifndef vtkSMRemoteObject_h
