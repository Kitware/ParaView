/*=========================================================================

  Program:   ParaView
  Module:    vtkSMContextArraysInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMContextArraysInformationHelper - gather array names for a table
// .SECTION Description
//
// This class iterates through a vtkTable and fills the supplied property with
// column names matching their current column index.

#ifndef __vtkSMContextArraysInformationHelper_h
#define __vtkSMContextArraysInformationHelper_h

#include "vtkSMInformationHelper.h"

class VTK_EXPORT vtkSMContextArraysInformationHelper : public vtkSMInformationHelper
{
public:
  static vtkSMContextArraysInformationHelper* New();
  vtkTypeMacro(vtkSMContextArraysInformationHelper, vtkSMInformationHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Updates the property using value obtained for server. It creates
  // an instance of the server helper class vtkPVServerArraySelection
  // and passes the objectId (which the helper class gets as a pointer)
  // and populates the property using the values returned.
  // Each array is represented by two components:
  // name, state (on/off)
  virtual void UpdateProperty(
    vtkIdType connectionId,
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop);
  //ETX

//BTX
protected:
  vtkSMContextArraysInformationHelper();
  ~vtkSMContextArraysInformationHelper();

private:
  vtkSMContextArraysInformationHelper(const vtkSMContextArraysInformationHelper&); // Not implemented
  void operator=(const vtkSMContextArraysInformationHelper&); // Not implemented
//ETX
};

#endif

