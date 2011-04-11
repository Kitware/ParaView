/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRemoteObjectUpdateUndoElement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRemoteObjectUpdateUndoElement - vtkSMRemoteObject undo element.
// .SECTION Description
// This class keeps the before and after state of the RemoteObject in the
// vtkSMMessage form. It works with any proxy and RemoteObject. It is a very
// generic undoElement.

#ifndef __vtkSMRemoteObjectUpdateUndoElement_h
#define __vtkSMRemoteObjectUpdateUndoElement_h

#include "vtkSMUndoElement.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage
#include "vtkWeakPointer.h" //  needed for vtkWeakPointer.

class vtkSMStateLocator;

class VTK_EXPORT vtkSMRemoteObjectUpdateUndoElement : public vtkSMUndoElement
{
public:
  static vtkSMRemoteObjectUpdateUndoElement* New();
  vtkTypeMacro(vtkSMRemoteObjectUpdateUndoElement, vtkSMUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Undo the operation encapsulated by this element.
  // \return the status of the operation, 1 on success, 0 otherwise.
  virtual int Undo();

  // Description:
  // Redo the operation encaspsulated by this element.
  // \return the status of the operation, 1 on success, 0 otherwise.
  virtual int Redo();

  // Description:
  // A specific StateLoctor can be used when the state gets loaded by the
  // remote object. Otherwise we use the one that is available in the session
  // that was used to create that undo element.
  virtual void SetStateLocator(vtkSMStateLocator* locator);

//BTX

  // Description:
  // Set the state of the UndoElement
  virtual void SetUndoRedoState(const vtkSMMessage* before,
                                const vtkSMMessage* after);

  // Current full state of the UndoElement
  vtkSMMessage* BeforeState;
  vtkSMMessage* AfterState;

  virtual vtkTypeUInt32 GetGlobalId();

protected:
  vtkSMRemoteObjectUpdateUndoElement();
  ~vtkSMRemoteObjectUpdateUndoElement();

  // Internal method used to update proxy state based on the state info
  int UpdateState(const vtkSMMessage* state);

  vtkWeakPointer<vtkSMStateLocator> Locator;

private:
  vtkSMRemoteObjectUpdateUndoElement(const vtkSMRemoteObjectUpdateUndoElement&); // Not implemented.
  void operator=(const vtkSMRemoteObjectUpdateUndoElement&); // Not implemented.

//ETX
};

#endif
