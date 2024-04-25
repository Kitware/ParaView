// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMVRCollaborationStyleProxy
 * @brief   a proxy for communicating our pose to collaborators
 *
 * vtkSMVRCollaborationStyleProxy is a vtkSMVRInteractorStyleProxy used
 * internally by CAVEInteraction plugin to convert select tracker
 * events into vtkEventDataDevice3D events and invoked as such.  Incoming
 * events are first filtered to ignore trackers that the user did not
 * configure as the head or heands (using SetHeadEventName(), etc).
 *
 * Invoked events are then observed by vtkVRCollaborationClient, and from
 * there, packaged up and broadcast to collaborators as avatar pose messages.
 */

#ifndef vtkSMVRCollaborationStyleProxy_h
#define vtkSMVRCollaborationStyleProxy_h

#include "vtkSMVRInteractorStyleProxy.h"
#include "vtkVRQueue.h"

#include <memory> // for std::unique_ptr

class QString;

class vtkSMVRCollaborationStyleProxy : public vtkSMVRInteractorStyleProxy
{
public:
  static vtkSMVRCollaborationStyleProxy* New();
  vtkTypeMacro(vtkSMVRCollaborationStyleProxy, vtkSMVRInteractorStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /*
   * Set event names used to associate tracker events with
   * avatar body parts. This class will only generate Move3D
   * events and invoke them when events have the names
   * selected here.
   */
  void SetHeadEventName(QString* eventName);
  void SetLeftHandEventName(QString* eventName);
  void SetRightHandEventName(QString* eventName);

protected:
  vtkSMVRCollaborationStyleProxy();
  ~vtkSMVRCollaborationStyleProxy() override;
  void HandleTracker(const vtkVREvent& event) override;

private:
  vtkSMVRCollaborationStyleProxy(
    const vtkSMVRCollaborationStyleProxy&) = delete;              // Not implemented.
  void operator=(const vtkSMVRCollaborationStyleProxy&) = delete; // Not implemented.

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internal;
};

#endif // vtkSMVRCollaborationStyleProxy_h
