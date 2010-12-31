/*=========================================================================

  Program:   ParaView
  Module:    vtkSProxyMUndoElement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyUndoElement - vtkSMProxy undo element.
// .SECTION Description
// This class deals with Proxy creation and deletion

#ifndef __vtkSMProxyUndoElement_h
#define __vtkSMProxyUndoElement_h

#include "vtkSMUndoElement.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage

class VTK_EXPORT vtkSMProxyUndoElement : public vtkSMUndoElement
{
public:
  static vtkSMProxyUndoElement* New();
  vtkTypeMacro(vtkSMProxyUndoElement, vtkSMUndoElement);
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
  // If set to true, the Redo action will create the proxy and the undo will
  // delete it.
  vtkSetMacro(CreateElement, bool);
  vtkGetMacro(CreateElement, bool);

//BTX

  // Description:
  // Set the state of the UndoElement
  virtual void SetCreationState(const vtkSMMessage* createState);

protected:
  vtkSMProxyUndoElement();
  ~vtkSMProxyUndoElement();

  // Internal method used to re-create a proxy based on its state
  int CreateProxy();

  // Internal method used to delete the proxy based on the state informations
  int DeleteProxy();

  // Current full state for the creation
  vtkSMMessage *State;

  // Undo type.
  bool CreateElement;

//ETX

private:
  vtkSMProxyUndoElement(const vtkSMProxyUndoElement&); // Not implemented.
  void operator=(const vtkSMProxyUndoElement&); // Not implemented.
};

#endif
