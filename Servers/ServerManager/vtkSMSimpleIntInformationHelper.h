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
// .NAME vtkSMSimpleIntInformationHelper -
// .SECTION Description
// .SECTION See Also

#ifndef __vtkSMSimpleIntInformationHelper_h
#define __vtkSMSimpleIntInformationHelper_h

#include "vtkSMInformationHelper.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

class vtkSMProperty;

class VTK_EXPORT vtkSMSimpleIntInformationHelper : public vtkSMInformationHelper
{
public:
  static vtkSMSimpleIntInformationHelper* New();
  vtkTypeRevisionMacro(vtkSMSimpleIntInformationHelper, vtkSMInformationHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  virtual void UpdateProperty(
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop);
  //ETX

protected:
  vtkSMSimpleIntInformationHelper();
  ~vtkSMSimpleIntInformationHelper();

private:
  vtkSMSimpleIntInformationHelper(const vtkSMSimpleIntInformationHelper&); // Not implemented
  void operator=(const vtkSMSimpleIntInformationHelper&); // Not implemented
};

#endif
