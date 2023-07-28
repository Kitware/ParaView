// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMRemoteObjectUpdateUndoElement
 * @brief   vtkSMRemoteObject undo element.
 *
 * This class keeps the before and after state of the RemoteObject in the
 * vtkSMMessage form. It works with any proxy and RemoteObject. It is a very
 * generic undoElement.
 */

#ifndef vtkSMRemoteObjectUpdateUndoElement_h
#define vtkSMRemoteObjectUpdateUndoElement_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMMessageMinimal.h"            // needed for vtkSMMessage
#include "vtkSMUndoElement.h"
#include "vtkWeakPointer.h" //  needed for vtkWeakPointer.

class vtkSMProxyLocator;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMRemoteObjectUpdateUndoElement : public vtkSMUndoElement
{
public:
  static vtkSMRemoteObjectUpdateUndoElement* New();
  vtkTypeMacro(vtkSMRemoteObjectUpdateUndoElement, vtkSMUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Undo the operation encapsulated by this element.
   * \return the status of the operation, 1 on success, 0 otherwise.
   */
  int Undo() override;

  /**
   * Redo the operation encaspsulated by this element.
   * \return the status of the operation, 1 on success, 0 otherwise.
   */
  int Redo() override;

  /**
   * Set ProxyLocator to use if any.
   */
  virtual void SetProxyLocator(vtkSMProxyLocator*);

  /**
   * Set the state of the UndoElement
   */
  virtual void SetUndoRedoState(const vtkSMMessage* before, const vtkSMMessage* after);

  // Current full state of the UndoElement
  vtkSMMessage* BeforeState;
  vtkSMMessage* AfterState;

  virtual vtkTypeUInt32 GetGlobalId();

protected:
  vtkSMRemoteObjectUpdateUndoElement();
  ~vtkSMRemoteObjectUpdateUndoElement() override;

  // Internal method used to update proxy state based on the state info
  int UpdateState(const vtkSMMessage* state);

  vtkSMProxyLocator* ProxyLocator;

private:
  vtkSMRemoteObjectUpdateUndoElement(const vtkSMRemoteObjectUpdateUndoElement&) = delete;
  void operator=(const vtkSMRemoteObjectUpdateUndoElement&) = delete;
};

#endif
