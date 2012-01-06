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
// .NAME vtkSMInsituStateLoader
// .SECTION Description
//

#ifndef __vtkSMInsituStateLoader_h
#define __vtkSMInsituStateLoader_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMStateLoader.h"

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMInsituStateLoader : public vtkSMStateLoader
{
public:
  static vtkSMInsituStateLoader* New();
  vtkTypeMacro(vtkSMInsituStateLoader, vtkSMStateLoader);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMInsituStateLoader();
  ~vtkSMInsituStateLoader();

  // Description:
  // Overridden to try to reuse existing proxies as much as possible.
  virtual vtkSMProxy* NewProxy(vtkTypeUInt32 id, vtkSMProxyLocator* locator);

private:
  vtkSMInsituStateLoader(const vtkSMInsituStateLoader&); // Not implemented
  void operator=(const vtkSMInsituStateLoader&); // Not implemented
//ETX
};

#endif
