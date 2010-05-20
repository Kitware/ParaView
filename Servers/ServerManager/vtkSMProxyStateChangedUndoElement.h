/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyStateChangedUndoElement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyStateChangedUndoElement
// .SECTION Description
// vtkSMProxyStateChangedUndoElement is an undo-redo element used for recording
// 'StateChanged' events fired by proxies.

#ifndef __vtkSMProxyStateChangedUndoElement_h
#define __vtkSMProxyStateChangedUndoElement_h

#include "vtkSMUndoElement.h"
class vtkSMProxy;

class VTK_EXPORT vtkSMProxyStateChangedUndoElement : public vtkSMUndoElement
{
public:
  static vtkSMProxyStateChangedUndoElement* New();
  vtkTypeMacro(vtkSMProxyStateChangedUndoElement, vtkSMUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Undo the operation encapsulated by this element.
  virtual int Undo()
    { return this->UndoRedo(false); }

  // Description:
  // Redo the operation encaspsulated by this element.
  virtual int Redo()
    { return this->UndoRedo(true); }

  // Description:
  // Returns if this element can load the xml state for the given element.
  virtual bool CanLoadState(vtkPVXMLElement*);

  // Description:
  void StateChanged(vtkSMProxy* proxy, vtkPVXMLElement* stateChange);

//BTX
protected:
  vtkSMProxyStateChangedUndoElement();
  ~vtkSMProxyStateChangedUndoElement();

  int UndoRedo(bool redo);

private:
  vtkSMProxyStateChangedUndoElement(const vtkSMProxyStateChangedUndoElement&); // Not implemented
  void operator=(const vtkSMProxyStateChangedUndoElement&); // Not implemented
//ETX
};

#endif

