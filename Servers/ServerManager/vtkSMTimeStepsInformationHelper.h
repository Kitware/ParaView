/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTimeStepsInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMTimeStepsInformationHelper - Gets the time step values from server 
// .SECTION Description
// vtkSMTimeStepsInformationHelper gets the time step values (using
// vtkPVServerTimeSteps) from server and fills the information property
//
// .SECTION See also
// vtkSMTimeRangeInformationHelper vtkPVServerTimeSteps

#ifndef __vtkSMTimeStepsInformationHelper_h
#define __vtkSMTimeStepsInformationHelper_h

#include "vtkSMInformationHelper.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

class vtkSMProperty;

class VTK_EXPORT vtkSMTimeStepsInformationHelper : public vtkSMInformationHelper
{
public:
  static vtkSMTimeStepsInformationHelper* New();
  vtkTypeMacro(vtkSMTimeStepsInformationHelper, vtkSMInformationHelper);
  void PrintSelf(ostream& os, vtkIndent indent);
  
//BTX
  // Description:
  // Updates the property using values obtained for server.
  virtual void UpdateProperty(vtkIdType connectionId,
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop);
//ETX

protected:
  vtkSMTimeStepsInformationHelper();
  ~vtkSMTimeStepsInformationHelper();

private:
  vtkSMTimeStepsInformationHelper(const vtkSMTimeStepsInformationHelper&); // Not implemented
  void operator=(const vtkSMTimeStepsInformationHelper&); // Not implemented
};

#endif
