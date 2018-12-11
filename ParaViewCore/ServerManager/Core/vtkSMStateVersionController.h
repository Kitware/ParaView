/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStateVersionController.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMStateVersionController
 *
 * vtkSMStateVersionController is used to convert the state XML from any
 * previously supported versions to the current version.
*/

#ifndef vtkSMStateVersionController_h
#define vtkSMStateVersionController_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkSMSession;

class vtkPVXMLElement;
class VTKPVSERVERMANAGERCORE_EXPORT vtkSMStateVersionController : public vtkSMObject
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
