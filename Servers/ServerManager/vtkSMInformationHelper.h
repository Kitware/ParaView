/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMInformationHelper -
// .SECTION Description
// .SECTION See Also

#ifndef __vtkSMInformationHelper_h
#define __vtkSMInformationHelper_h

#include "vtkSMObject.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

class vtkSMProperty;

class VTK_EXPORT vtkSMInformationHelper : public vtkSMObject
{
public:
  vtkTypeRevisionMacro(vtkSMInformationHelper, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  virtual void UpdateProperty(
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop) = 0;
  //ETX

protected:
  vtkSMInformationHelper();
  ~vtkSMInformationHelper();

private:
  vtkSMInformationHelper(const vtkSMInformationHelper&); // Not implemented
  void operator=(const vtkSMInformationHelper&); // Not implemented
};

#endif
