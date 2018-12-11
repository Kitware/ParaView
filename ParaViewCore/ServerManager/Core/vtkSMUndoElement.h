/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUndoElement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMUndoElement
 * @brief   abstract superclass for Server Manager undo
 * elements.
 *
 * Abstract superclass for Server Manager undo elements.
 * This class keeps the session, so undoelement could work across a set of
 * communication Sessions.
*/

#ifndef vtkSMUndoElement_h
#define vtkSMUndoElement_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkUndoElement.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkSMSession;
class vtkSMSessionProxyManager;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMUndoElement : public vtkUndoElement
{
public:
  vtkTypeMacro(vtkSMUndoElement, vtkUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Get/Set the Session that has been used to generate that undoElement.
  virtual vtkSMSession* GetSession();
  virtual void SetSession(vtkSMSession*);

  /**
   * Return the corresponding ProxyManager if any.
   */
  virtual vtkSMSessionProxyManager* GetSessionProxyManager();

protected:
  vtkSMUndoElement();
  ~vtkSMUndoElement() override;

  // Identifies the session to which this object is related.
  vtkWeakPointer<vtkSMSession> Session;

private:
  vtkSMUndoElement(const vtkSMUndoElement&) = delete;
  void operator=(const vtkSMUndoElement&) = delete;
};

#endif
