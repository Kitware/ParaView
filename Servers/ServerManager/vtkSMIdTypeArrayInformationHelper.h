/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIdTypeArrayInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMIdTypeArrayInformationHelper - information helper to populate
// a vtkSMIdTypeVectorProperty with the results of its command which 
// returns a vtkIdTypeArray.
// .SECTION Description
// vtkSMIdTypeArrayInformationHelper only works with vtkSMIdTypeVectorProperty.
// It calls the property's Command on the root node on the associate server.
// The command must returns a vtkIdTypeArray. The helper populates the
// property with the values in the vtkIdTypeArray.

#ifndef __vtkSMIdTypeArrayInformationHelper_h
#define __vtkSMIdTypeArrayInformationHelper_h

#include "vtkSMInformationHelper.h"

class VTK_EXPORT vtkSMIdTypeArrayInformationHelper : 
  public vtkSMInformationHelper
{
public:
  static vtkSMIdTypeArrayInformationHelper* New();
  vtkTypeMacro(vtkSMIdTypeArrayInformationHelper, vtkSMInformationHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Updates the property using value obtained from the server. It creates
  // an instance of the server helper class vtkPVServerArraySelection
  // and passes the objectId (which the helper class gets as a pointer)
  // and populates the property using the values returned.
  // Each array is represented by two components:
  // name, state (on/off)  
  virtual void UpdateProperty(
    vtkIdType connectionId, int serverIds, 
    vtkClientServerID objectId, vtkSMProperty* prop);
  //ETX
protected:
  vtkSMIdTypeArrayInformationHelper();
  ~vtkSMIdTypeArrayInformationHelper();

private:
  vtkSMIdTypeArrayInformationHelper(const vtkSMIdTypeArrayInformationHelper&); // Not implemented.
  void operator=(const vtkSMIdTypeArrayInformationHelper&); // Not implemented.

};


#endif

