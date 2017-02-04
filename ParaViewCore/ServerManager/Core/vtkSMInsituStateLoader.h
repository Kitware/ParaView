/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMInsituStateLoader
 *
 *
*/

#ifndef vtkSMInsituStateLoader_h
#define vtkSMInsituStateLoader_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMStateLoader.h"

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMInsituStateLoader : public vtkSMStateLoader
{
public:
  static vtkSMInsituStateLoader* New();
  vtkTypeMacro(vtkSMInsituStateLoader, vtkSMStateLoader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkSMInsituStateLoader();
  ~vtkSMInsituStateLoader();

  /**
   * Overridden to try to reuse existing proxies as much as possible.
   */
  virtual vtkSMProxy* NewProxy(vtkTypeUInt32 id, vtkSMProxyLocator* locator) VTK_OVERRIDE;

private:
  vtkSMInsituStateLoader(const vtkSMInsituStateLoader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMInsituStateLoader&) VTK_DELETE_FUNCTION;
};

#endif
