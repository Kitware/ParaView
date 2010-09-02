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
#include "vtkWeakPointer.h"
#include "vtkSMMessage.h"

class vtkClientServerInterpreter;
class vtkSMSessionCore;

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
  virtual void Push(vtkSMMessage* msg) = 0;

  // Description:
  // Pull the current state of the underneath implementation
  virtual void Pull(vtkSMMessage* msg) = 0;

  // Description:
  // Invoke a given method on the underneath objects
  virtual void Invoke(vtkSMMessage* msg) = 0;
//ETX

  // Description:
  /// Method called when the vtkPMObject should be deleted and should release
  /// its internal content
  virtual void Finalize();

//BTX

  // Description:
  // Provides access to the Interpreter.
  vtkClientServerInterpreter* GetInterpreter();

  // Description:
  // Convenience method to obtain a vtkPMObject subclass given its global id.
  vtkPMObject* GetPMObject(vtkTypeUInt32 globalid);

protected:
  vtkPMObject();
  virtual ~vtkPMObject();

  vtkWeakPointer<vtkClientServerInterpreter> Interpreter;
  vtkWeakPointer<vtkSMSessionCore> SessionCore;

private:
  vtkPMObject(const vtkPMObject&);    // Not implemented
  void operator=(const vtkPMObject&); // Not implemented
//ETX
};

#endif // #ifndef __vtkPMObject_h
