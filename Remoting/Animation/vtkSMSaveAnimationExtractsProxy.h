// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSMSaveAnimationExtractsProxy
 * @brief generate extracts using animation scene
 *
 * vtkSMSaveAnimationExtractsProxy is used to generate extracts by using the
 * animation scene to drive the pipeline updates. Essentially, this will play
 * through the animation scene associated with the session for this proxy and
 * for each frame in the animation, generate extracts based on the extract
 * generators defined.
 *
 * As with any other vtkSMProxy and subclass, one does not directly instantiate
 * this class instead use `vtkSMSessionProxyManager::NewProxy`.
 * One definition for this proxy is under proxy-group `"misc"` and
 * proxy-name `"SaveAnimationExtracts"`.
 */

#ifndef vtkSMSaveAnimationExtractsProxy_h
#define vtkSMSaveAnimationExtractsProxy_h

#include "vtkRemotingAnimationModule.h" //needed for exports
#include "vtkSMProxy.h"

class VTKREMOTINGANIMATION_EXPORT vtkSMSaveAnimationExtractsProxy : public vtkSMProxy
{
public:
  static vtkSMSaveAnimationExtractsProxy* New();
  vtkTypeMacro(vtkSMSaveAnimationExtractsProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Generates extracts.
   */
  bool SaveExtracts();

protected:
  vtkSMSaveAnimationExtractsProxy();
  ~vtkSMSaveAnimationExtractsProxy() override;

private:
  vtkSMSaveAnimationExtractsProxy(const vtkSMSaveAnimationExtractsProxy&) = delete;
  void operator=(const vtkSMSaveAnimationExtractsProxy&) = delete;
};

#endif
