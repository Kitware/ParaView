/*=========================================================================

  Program:   ParaView
  Module:    vtkPMObject.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPMObject
// .SECTION Description
// Object that is managed by process module which wrap concrete class such as
// the vtk ones.
// FIXME: WE BADLY NEED TO FIND A GOOD NAME FOR THIS CLASS.

#ifndef __vtkPMObject_h
#define __vtkPMObject_h

#include "vtkSMObject.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage
#include "vtkWeakPointer.h" // needed for vtkWeakPointer

class vtkClientServerInterpreter;
class vtkSMSessionCore;
class vtkSMRemoteObject;

class VTK_EXPORT vtkPMObject : public vtkSMObject
{
public:
  vtkTypeMacro(vtkPMObject,vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initializes the instance. Session is the session to which this instance
  // belongs to. During initialization, the PMObject basically obtains ivars for
  // necessary components.
  virtual void Initialize(vtkSMSessionCore* session);

//BTX
  // Description:
  // Push a new state to the underneath implementation
  // The provided implementation just store the message
  // and return it at the Pull one.
  virtual void Push(vtkSMMessage* msg);

  // Description:
  // Pull the current state of the underneath implementation
  // The provided implementation update the given message with the one
  // that has been previously pushed
  virtual void Pull(vtkSMMessage* msg);

  // Description:
  // Invoke a given method on the underneath objects
  // The provided implementation is Empty and do nothing
  virtual void Invoke(vtkSMMessage* msg);
//ETX

//BTX

  // Description:
  // Provides access to the Interpreter.
  vtkClientServerInterpreter* GetInterpreter();

  // Description:
  // Convenience method to obtain a vtkPMObject subclass given its global id.
  vtkPMObject* GetPMObject(vtkTypeUInt32 globalid) const;

  // Description:
  // Convenience method to obtain a vtkSMRemoteObject subclass given its
  // global id.
  vtkSMRemoteObject* GetRemoteObject(vtkTypeUInt32 globalid);

  // Description:
  // Get/Set the global id for this object.
  vtkSetMacro(GlobalID, vtkTypeUInt32);
  vtkGetMacro(GlobalID, vtkTypeUInt32);

protected:
  vtkPMObject();
  virtual ~vtkPMObject();

  vtkWeakPointer<vtkClientServerInterpreter> Interpreter;
  vtkWeakPointer<vtkSMSessionCore> SessionCore;

  vtkSMMessage* LastPushedMessage;

  vtkTypeUInt32 GlobalID;
private:
  vtkPMObject(const vtkPMObject&);    // Not implemented
  void operator=(const vtkPMObject&); // Not implemented
//ETX
};

#endif // #ifndef __vtkPMObject_h
