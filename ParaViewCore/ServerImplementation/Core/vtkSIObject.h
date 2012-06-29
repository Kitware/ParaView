/*=========================================================================

  Program:   ParaView
  Module:    vtkSIObject.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIObject
// .SECTION Description
// Object that is managed by vtkPVSessionCore which wrap concrete class such as
// the vtk ones.

#ifndef __vtkSIObject_h
#define __vtkSIObject_h

#include "vtkObject.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage
#include "vtkWeakPointer.h" // needed for vtkWeakPointer

class vtkClientServerInterpreter;
class vtkPVSessionCore;

class VTK_EXPORT vtkSIObject : public vtkObject
{
public:
  static vtkSIObject* New();
  vtkTypeMacro(vtkSIObject,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method is called before the deletion of the SIObject.
  // Basically this is used to remove all dependency with other SIObject so after
  // a first pass on all SIObject with a AboutToDelete() we can simply delete the
  // remaining SIObjects.
  virtual void AboutToDelete() {};

  // Description:
  // Initializes the instance. Session is the session to which this instance
  // belongs to. During initialization, the SIObject basically obtains ivars for
  // necessary components.
  virtual void Initialize(vtkPVSessionCore* session);

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
//ETX

//BTX

  // Description:
  // Provides access to the Interpreter.
  vtkClientServerInterpreter* GetInterpreter();

  // Description:
  // Convenience method to obtain a vtkSIObject subclass given its global id.
  vtkSIObject* GetSIObject(vtkTypeUInt32 globalid) const;

  // Description:
  // Convenience method to obtain a vtkObject subclass given its
  // global id.
  vtkObject* GetRemoteObject(vtkTypeUInt32 globalid);

  // Description:
  // Get/Set the global id for this object.
  vtkSetMacro(GlobalID, vtkTypeUInt32);
  vtkGetMacro(GlobalID, vtkTypeUInt32);

protected:
  vtkSIObject();
  virtual ~vtkSIObject();

  vtkWeakPointer<vtkClientServerInterpreter> Interpreter;
  vtkWeakPointer<vtkPVSessionCore> SessionCore;

  vtkSMMessage* LastPushedMessage;

  vtkTypeUInt32 GlobalID;
private:
  vtkSIObject(const vtkSIObject&);    // Not implemented
  void operator=(const vtkSIObject&); // Not implemented
//ETX
};

#endif // #ifndef __vtkSIObject_h
