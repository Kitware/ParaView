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
// This class keeps the before and after state of the RemoteObject in the vtkSMMessage
// form.

#ifndef __vtkSMRemoteObjectUpdateUndoElement_h
#define __vtkSMRemoteObjectUpdateUndoElement_h

#include "vtkSMUndoElement.h"
#include "vtkSMMessage.h"

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

//BTX

  // Description:
  // Set the state of the UndoElement
  virtual void SetUndoRedoState(const vtkSMMessage* before,
                                const vtkSMMessage* after);

  // Current full state of the UndoElement
  vtkSMMessage BeforeState;
  vtkSMMessage AfterState;

protected:
  vtkSMRemoteObjectUpdateUndoElement();
  ~vtkSMRemoteObjectUpdateUndoElement();

  // Internal method used to update proxy state based on the state info
  int UpdateState(const vtkSMMessage* state);

private:
  vtkSMRemoteObjectUpdateUndoElement(const vtkSMRemoteObjectUpdateUndoElement&); // Not implemented.
  void operator=(const vtkSMRemoteObjectUpdateUndoElement&); // Not implemented.

//ETX
};

#endif
