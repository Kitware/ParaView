/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStateLocator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMStateLocator - Class used to retreive a given message state based
// on its GlobalID.
// .SECTION Description
// vtkSMStateLocator allow a hierarchical way of finding a message state.

#ifndef __vtkSMStateLocator_h
#define __vtkSMStateLocator_h

#include "vtkSMObject.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage.
#include "vtkWeakPointer.h" // need for observer

class vtkSMProxy;
class vtkSMSession;
class vtkUndoStack;

class VTK_EXPORT vtkSMStateLocator : public vtkSMObject
{
public:
  static vtkSMStateLocator* New();
  vtkTypeMacro(vtkSMStateLocator, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get a parent locator to search which is used as a backup location
  // to search from if a given state was not found locally.
  vtkSetObjectMacro(ParentLocator, vtkSMStateLocator);
  vtkGetObjectMacro(ParentLocator, vtkSMStateLocator);

  // Description:
  // By initializing the garabage collector the stored state get removed once
  // their is no more chance for them to be reused inside the session.
  void InitGarbageCollector(vtkSMSession*, vtkUndoStack*);

//BTX
  // Description:
  // Fill the provided State message with the state found inside the current
  // locator or one of its parent. The method return true if the state was
  // successfully filled.
  // The "useParent" flag allow to disable parent lookup but by default it
  // is set to true.
  virtual bool FindState(vtkTypeUInt32 globalID, vtkSMMessage* stateToFill,
                         bool useParent = true );

  // Description:
  // Register the given state in the current locator. If a previous state was
  // available, the new one will replace it.
  virtual void RegisterState(const vtkSMMessage* state);

  // Description:
  // Remove a state for a given proxy inside the local locator.
  // if force is true, it will also remove it from its hierarchical parents.
  virtual void UnRegisterState(vtkTypeUInt32 globalID, bool force);

  // Description:
  // Remove all the registered states
  // if force is true, it will also remove it from its hierarchical parents.
  virtual void UnRegisterAllStates(bool force);

  // Description:
  // Return true if the given state can be found locally whitout the help of
  // on the hierarchical parent
  virtual bool IsStateLocal(vtkTypeUInt32 globalID);

  // Description:
  // Return true if the given state do exist in the locator hierachy
  virtual bool IsStateAvailable(vtkTypeUInt32 globalID);

  // Description:
  // Register the given proxy state as well as all its sub-proxy state so if
  // that proxy need to be renew all its sub-proxy will be renew in the exact
  // same state.
  virtual void RegisterFullState(vtkSMProxy* proxy);

protected:
  vtkSMStateLocator();
  ~vtkSMStateLocator();

  vtkSMStateLocator* ParentLocator;
  vtkWeakPointer<vtkSMSession> Session;
  vtkWeakPointer<vtkUndoStack> UndoStack;

private:
  vtkSMStateLocator(const vtkSMStateLocator&); // Not implemented
  void operator=(const vtkSMStateLocator&); // Not implemented

  class vtkInternal;
  vtkInternal* Internals;
//ETX
};

#endif
