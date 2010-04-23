/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyUnRegisterUndoElement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyUnRegisterUndoElement
// .SECTION Description
// This is a concrete implementation of a Undo element for a proxy unregister event.
// The undo action creates a new proxy if the proxy to register has been deleted
// and updates its state to the one before it was unregistered.
// The redo action unregisters the proxy. As a consequence, the proxy may get deleted.
// \b Note that Undo/Redo are not idempotent operations.

#ifndef __vtkSMProxyUnRegisterUndoElement_h
#define __vtkSMProxyUnRegisterUndoElement_h

#include "vtkSMUndoElement.h"

class vtkSMProxy;

class VTK_EXPORT vtkSMProxyUnRegisterUndoElement : public vtkSMUndoElement
{
public:
  static vtkSMProxyUnRegisterUndoElement* New();
  vtkTypeMacro(vtkSMProxyUnRegisterUndoElement, vtkSMUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Undo the operation encapsulated by this element.
  virtual int Undo();

  // Description:
  // Redo the operation encaspsulated by this element.
  virtual int Redo();

  // Description:
  // Returns if this element can load the xml state for the given element.
  virtual bool CanLoadState(vtkPVXMLElement*);

  // Description:
  // Sets the information about the proxy that is getting unregistered.
  virtual void ProxyToUnRegister(const char* groupname, const char* proxyname, 
    vtkSMProxy* proxy);
protected:
  vtkSMProxyUnRegisterUndoElement();
  ~vtkSMProxyUnRegisterUndoElement();

private:
  vtkSMProxyUnRegisterUndoElement(const vtkSMProxyUnRegisterUndoElement&); // Not implemented.
  void operator=(const vtkSMProxyUnRegisterUndoElement&); // Not implemented.
};

#endif

