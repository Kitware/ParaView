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
// .NAME vtkSMSimpleStringInformationHelper -
// .SECTION Description
// .SECTION See Also

#ifndef __vtkSMSimpleStringInformationHelper_h
#define __vtkSMSimpleStringInformationHelper_h

#include "vtkSMInformationHelper.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

class vtkSMProperty;

class VTK_EXPORT vtkSMSimpleStringInformationHelper : public vtkSMInformationHelper
{
public:
  static vtkSMSimpleStringInformationHelper* New();
  vtkTypeRevisionMacro(vtkSMSimpleStringInformationHelper, vtkSMInformationHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  virtual void UpdateProperty(
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop);
  //ETX

protected:
  vtkSMSimpleStringInformationHelper();
  ~vtkSMSimpleStringInformationHelper();

private:
  vtkSMSimpleStringInformationHelper(const vtkSMSimpleStringInformationHelper&); // Not implemented
  void operator=(const vtkSMSimpleStringInformationHelper&); // Not implemented
};

#endif
