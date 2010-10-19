/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyUpdateUndoElement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyUpdateUndoElement - vtkSMProxy undo element.
// .SECTION Description
// This class keeps the before and after state of the Proxy in the vtkSMMessage
// form.

#ifndef __vtkSMProxyUpdateUndoElement_h
#define __vtkSMProxyUpdateUndoElement_h

#include "vtkSMUndoElement.h"
#include "vtkSMMessage.h"

class VTK_EXPORT vtkSMProxyUpdateUndoElement : public vtkSMUndoElement
{
public:
  static vtkSMProxyUpdateUndoElement* New();
  vtkTypeMacro(vtkSMProxyUpdateUndoElement, vtkSMUndoElement);
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

protected:
  vtkSMProxyUpdateUndoElement();
  ~vtkSMProxyUpdateUndoElement();

  // Internal method used to update proxy state based on the state info
  int UpdateProxyState(const vtkSMMessage* state);

  // Current full state of the UndoElement
  vtkSMMessage BeforeState;
  vtkSMMessage AfterState;

//ETX

private:
  vtkSMProxyUpdateUndoElement(const vtkSMProxyUpdateUndoElement&); // Not implemented.
  void operator=(const vtkSMProxyUpdateUndoElement&); // Not implemented.
};

#endif
