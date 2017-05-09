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

class vtkPVXMLElement;
class VTKPVSERVERMANAGERCORE_EXPORT vtkSMStateVersionController : public vtkSMObject
{
public:
  static vtkSMStateVersionController* New();
  vtkTypeMacro(vtkSMStateVersionController, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Called before a state is loaded.
   * The argument must be the root element for the state being loaded.
   * eg. for server manager state, it will point to \c \<ServerManagerState/\>
   * element.
   * Returns false if the conversion failed, else true.
   */
  virtual bool Process(vtkPVXMLElement* root);

protected:
  vtkSMStateVersionController();
  ~vtkSMStateVersionController();

private:
  vtkSMStateVersionController(const vtkSMStateVersionController&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMStateVersionController&) VTK_DELETE_FUNCTION;
};

#endif
