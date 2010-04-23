/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTimeRangeInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMTimeRangeInformationHelper - Gets the time range from server 
// .SECTION Description
// vtkSMTimeRangeInformationHelper gets the time range (using
// vtkPVServerTimeSteps) from server and fills the information property
//
// .SECTION See also
// vtkSMTimeStepsInformationHelper vtkPVServerTimeSteps

#ifndef __vtkSMTimeRangeInformationHelper_h
#define __vtkSMTimeRangeInformationHelper_h

#include "vtkSMInformationHelper.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

class vtkSMProperty;

class VTK_EXPORT vtkSMTimeRangeInformationHelper : public vtkSMInformationHelper
{
public:
  static vtkSMTimeRangeInformationHelper* New();
  vtkTypeMacro(vtkSMTimeRangeInformationHelper, vtkSMInformationHelper);
  void PrintSelf(ostream& os, vtkIndent indent);
  
//BTX
  // Description:
  // Updates the property using values obtained for server.
  virtual void UpdateProperty(vtkIdType connectionId,
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop);
//ETX

protected:
  vtkSMTimeRangeInformationHelper();
  ~vtkSMTimeRangeInformationHelper();

private:
  vtkSMTimeRangeInformationHelper(const vtkSMTimeRangeInformationHelper&); // Not implemented
  void operator=(const vtkSMTimeRangeInformationHelper&); // Not implemented
};

#endif
