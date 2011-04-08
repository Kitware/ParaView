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
/// .NAME pqProxyModifiedStateUndoElement - undo element to capture the change
/// in the modified state for a pqProxy.
/// .SECTION Description
/// pqProxyModifiedStateUndoElement can be used to capture the change in the
/// modified state of a pqProxy. Currently it only captures the change from
/// UNINITIALIZED to UNMODIFIED or vice-versa.  This is used by the
/// pqObjectInspectorWidget to control the apply button state when the first
/// accept is undone.

#ifndef __pqProxyModifiedStateUndoElement_h
#define __pqProxyModifiedStateUndoElement_h

#include "vtkSMUndoElement.h"
#include "pqCoreExport.h"

class pqProxy;

class PQCORE_EXPORT pqProxyModifiedStateUndoElement : public vtkSMUndoElement
{
public:
  static pqProxyModifiedStateUndoElement* New();
  vtkTypeMacro(pqProxyModifiedStateUndoElement, vtkSMUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual int Undo()
    {
    return this->InternalUndoRedo(true)? 1 : 0;
    }
  virtual int Redo()
    {
    return this->InternalUndoRedo(false)? 1 : 0;
    }

  /// Use this to initialize the element if the pqProxy was marked as
  /// UNMODIFIED.
  void MadeUnmodified(pqProxy*);

  /// Use this to initialize the element if the pqProxy was marked as
  /// UNINITIALIZED.
  void MadeUninitialized(pqProxy*);

//BTX
protected:
  pqProxyModifiedStateUndoElement();
  ~pqProxyModifiedStateUndoElement();

  bool InternalUndoRedo(bool undo);
  vtkTypeUInt32 ProxySourceGlobalId;
  bool Reverse;
private:
  pqProxyModifiedStateUndoElement(const pqProxyModifiedStateUndoElement&); // Not implemented
  void operator=(const pqProxyModifiedStateUndoElement&); // Not implemented
//ETX
};

#endif

