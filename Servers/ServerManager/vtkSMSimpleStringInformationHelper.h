/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSimpleStringInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSimpleStringInformationHelper - populates vtkSMStringVectorProperty with the result of it's command
// .SECTION Description
// vtkSMSimpleStringInformationHelper only work with
// vtkSMStringVectorProperties. It calls the property's Command on the
// root node of the associated server and populates the property using
// the values returned.
// .SECTION See Also
// vtkSMInformationHelper vtkSMStringVectorPropertiy

#ifndef __vtkSMSimpleStringInformationHelper_h
#define __vtkSMSimpleStringInformationHelper_h

#include "vtkSMInformationHelper.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

class vtkSMProperty;

class VTK_EXPORT vtkSMSimpleStringInformationHelper : public vtkSMInformationHelper
{
public:
  static vtkSMSimpleStringInformationHelper* New();
  vtkTypeMacro(vtkSMSimpleStringInformationHelper, vtkSMInformationHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Updates the property using value obtained for server. Calls
  // property's Command on the root node of the server and uses the
  // return value.
  virtual void UpdateProperty(vtkIdType connectionId,  int serverIds, 
    vtkClientServerID objectId, vtkSMProperty* prop);
  //ETX

protected:
  vtkSMSimpleStringInformationHelper();
  ~vtkSMSimpleStringInformationHelper();

private:
  vtkSMSimpleStringInformationHelper(const vtkSMSimpleStringInformationHelper&); // Not implemented
  void operator=(const vtkSMSimpleStringInformationHelper&); // Not implemented
};

#endif
