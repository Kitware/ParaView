/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSimpleIntInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSimpleIntInformationHelper - populates vtkSMIntVectorProperty with the results of it's command
// .SECTION Description
// vtkSMSimpleIntInformationHelper only works with 
// vtkSMIntVectorProperties. It calls the property's Command on the
// root node of the associated server and populates the property using
// the values returned.
// .SECTION See Also
// vtkSMInformationHelper vtkSMIntVectorProperty

#ifndef __vtkSMSimpleIntInformationHelper_h
#define __vtkSMSimpleIntInformationHelper_h

#include "vtkSMInformationHelper.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

class vtkSMProperty;

class VTK_EXPORT vtkSMSimpleIntInformationHelper : public vtkSMInformationHelper
{
public:
  static vtkSMSimpleIntInformationHelper* New();
  vtkTypeMacro(vtkSMSimpleIntInformationHelper, vtkSMInformationHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Updates the property using values obtained for server. Calls
  // property's Command on the root node of the server and uses the
  // return value(s).
  virtual void UpdateProperty(vtkIdType connectionId, int serverIds, 
    vtkClientServerID objectId, vtkSMProperty* prop);
  //ETX

protected:
  vtkSMSimpleIntInformationHelper();
  ~vtkSMSimpleIntInformationHelper();

private:
  vtkSMSimpleIntInformationHelper(const vtkSMSimpleIntInformationHelper&); // Not implemented
  void operator=(const vtkSMSimpleIntInformationHelper&); // Not implemented
};

#endif
