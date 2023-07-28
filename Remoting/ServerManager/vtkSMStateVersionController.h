// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMStateVersionController
 *
 * vtkSMStateVersionController is used to convert the state XML from any
 * previously supported versions to the current version.
 */

#ifndef vtkSMStateVersionController_h
#define vtkSMStateVersionController_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkSMSession;

class vtkPVXMLElement;
class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMStateVersionController : public vtkSMObject
{
public:
  static vtkSMStateVersionController* New();
  vtkTypeMacro(vtkSMStateVersionController, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Called before a state is loaded.
   * \param root Root element for the state being loaded, e.g. for server manager
   * state, it will point to a \c \<ServerManagerState/\> element.
   * \param session The current session, null by default.
   *
   * \return false if the conversion failed, else true.
   */
  virtual bool Process(vtkPVXMLElement* root, vtkSMSession* session = nullptr);

protected:
  vtkSMStateVersionController();
  ~vtkSMStateVersionController() override;

private:
  vtkSMStateVersionController(const vtkSMStateVersionController&) = delete;
  void operator=(const vtkSMStateVersionController&) = delete;
};

#endif
