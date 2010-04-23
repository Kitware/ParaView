/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIntArrayInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMIntArrayInformationHelper - information helper to populate
// a vtkSMIntVectorProperty with the results of its command which 
// returns a vtkIntArray.
// .SECTION Description
// vtkSMIntArrayInformationHelper only works with vtkSMIntVectorProperty.
// It calls the property's Command on the root node on the associate server.
// The command must returns a vtkIntArray. The helper the populates the
// property with the values in the vtkIntArray.

#ifndef __vtkSMIntArrayInformationHelper_h
#define __vtkSMIntArrayInformationHelper_h

#include "vtkSMInformationHelper.h"

class VTK_EXPORT vtkSMIntArrayInformationHelper : public vtkSMInformationHelper
{
public:
  static vtkSMIntArrayInformationHelper* New();
  vtkTypeMacro(vtkSMIntArrayInformationHelper, vtkSMInformationHelper);
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
protected:
  vtkSMIntArrayInformationHelper();
  ~vtkSMIntArrayInformationHelper();

private:
  vtkSMIntArrayInformationHelper(const vtkSMIntArrayInformationHelper&); // Not implemented.
  void operator=(const vtkSMIntArrayInformationHelper&); // Not implemented.

};


#endif

