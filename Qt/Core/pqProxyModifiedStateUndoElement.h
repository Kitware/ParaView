/*=========================================================================

  Program:   ParaView
  Module:    pqProxyModifiedStateUndoElement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
* .NAME pqProxyModifiedStateUndoElement - undo element to capture the change
* in the modified state for a pqProxy.
* .SECTION Description
* pqProxyModifiedStateUndoElement can be used to capture the change in the
* modified state of a pqProxy. Currently it only captures the change from
* UNINITIALIZED to UNMODIFIED or vice-versa.  This is used by the
* pqObjectInspectorWidget to control the apply button state when the first
* accept is undone.
*/

#ifndef pqProxyModifiedStateUndoElement_h
#define pqProxyModifiedStateUndoElement_h

#include "pqCoreModule.h"
#include "vtkSMUndoElement.h"

class pqProxy;

class PQCORE_EXPORT pqProxyModifiedStateUndoElement : public vtkSMUndoElement
{
public:
  static pqProxyModifiedStateUndoElement* New();
  vtkTypeMacro(pqProxyModifiedStateUndoElement, vtkSMUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int Undo() override { return this->InternalUndoRedo(true) ? 1 : 0; }
  int Redo() override { return this->InternalUndoRedo(false) ? 1 : 0; }

  /**
  * Use this to initialize the element if the pqProxy was marked as
  * UNMODIFIED.
  */
  void MadeUnmodified(pqProxy*);

  /**
  * Use this to initialize the element if the pqProxy was marked as
  * UNINITIALIZED.
  */
  void MadeUninitialized(pqProxy*);

protected:
  pqProxyModifiedStateUndoElement();
  ~pqProxyModifiedStateUndoElement() override;

  bool InternalUndoRedo(bool undo);
  vtkTypeUInt32 ProxySourceGlobalId;
  bool Reverse;

private:
  pqProxyModifiedStateUndoElement(const pqProxyModifiedStateUndoElement&) = delete;
  void operator=(const pqProxyModifiedStateUndoElement&) = delete;
};

#endif
