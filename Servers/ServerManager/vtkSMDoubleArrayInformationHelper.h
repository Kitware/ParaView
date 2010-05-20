/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDoubleArrayInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDoubleArrayInformationHelper - information helper to populate
// a vtkSMDoubleVectorProperty with the results of its command which 
// returns a vtkDoubleArray.
// .SECTION Description
// vtkSMDoubleArrayInformationHelper only works with vtkSMDoubleVectorProperty.
// It calls the property's Command on the root node on the associate server.
// The command must returns a vtkDoubleArray. The helper the populates the
// property with the values in the vtkDoubleArray.

#ifndef __vtkSMDoubleArrayInformationHelper_h
#define __vtkSMDoubleArrayInformationHelper_h

#include "vtkSMInformationHelper.h"

class VTK_EXPORT vtkSMDoubleArrayInformationHelper : public vtkSMInformationHelper
{
public:
  static vtkSMDoubleArrayInformationHelper* New();
  vtkTypeMacro(vtkSMDoubleArrayInformationHelper, vtkSMInformationHelper);
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
  vtkSMDoubleArrayInformationHelper();
  ~vtkSMDoubleArrayInformationHelper();

private:
  vtkSMDoubleArrayInformationHelper(const vtkSMDoubleArrayInformationHelper&); // Not implemented.
  void operator=(const vtkSMDoubleArrayInformationHelper&); // Not implemented.

};


#endif

