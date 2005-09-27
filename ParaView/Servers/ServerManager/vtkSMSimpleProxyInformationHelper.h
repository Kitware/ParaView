/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSimpleProxyInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSimpleProxyInformationHelper - populates vtkSMProxyProperty with the results of its command
// .SECTION Description
// vtkSMSimpleProxyInformationHelper only works with 
// vtkSMProxyProperties. It calls the property's command on the 
// root node of the associated server and populates the property
// using the value(s) returned.
// .SECTION See Also
// vtkSMInformationHelper vtkSMProxyProperty

#ifndef __vtkSMSimpleProxyInformationHelper_h
#define __vtkSMSimpleProxyInformationHelper_h

#include "vtkSMInformationHelper.h"

class VTK_EXPORT vtkSMSimpleProxyInformationHelper : public vtkSMInformationHelper
{
public:
  static vtkSMSimpleProxyInformationHelper* New();
  vtkTypeRevisionMacro(vtkSMSimpleProxyInformationHelper,vtkSMInformationHelper);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  //BTX
  // Description:
  // Updates the property using values obtained for server. Calls
  // property's Command on the root node of the server and uses the
  // return value(s).
  virtual void UpdateProperty(
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop);
  //ETX
protected:
  vtkSMSimpleProxyInformationHelper();
  ~vtkSMSimpleProxyInformationHelper();

private:
  vtkSMSimpleProxyInformationHelper(const vtkSMSimpleProxyInformationHelper&); // Not implemented
  void operator=(const vtkSMSimpleProxyInformationHelper&); // Not implemented
};

#endif

