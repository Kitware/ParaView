/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyRegisterUndoElement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyRegisterUndoElement
// .SECTION Description
// This is a concrete implementation of a Undo element for a proxy register event.
// The undo action unregisters the proxy, as a consequence of which, the proxy
// may get deleted.
// The redo action creates the proxy, if it no longer exists and registers it.
// \b Note that Undo/Redo are not idempotent operations.

#ifndef __vtkSMProxyRegisterUndoElement_h
#define __vtkSMProxyRegisterUndoElement_h

#include "vtkSMUndoElement.h"

class vtkSMProxy;


class VTK_EXPORT vtkSMProxyRegisterUndoElement : public vtkSMUndoElement
{
public:
  static vtkSMProxyRegisterUndoElement* New();
  vtkTypeMacro(vtkSMProxyRegisterUndoElement, vtkSMUndoElement);
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
  // Set the information about the proxy that is getting registered.
  void ProxyToRegister(const char* groupname, const char* proxyname,
    vtkSMProxy* proxy);
//BTX  
protected:
  vtkSMProxyRegisterUndoElement();
  ~vtkSMProxyRegisterUndoElement();


private:
  vtkSMProxyRegisterUndoElement(const vtkSMProxyRegisterUndoElement&); // Not implemented.
  void operator=(const vtkSMProxyRegisterUndoElement&); // Not implemented.
//ETX
};

#endif

