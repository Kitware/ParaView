/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSimpleIdTypeInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSimpleIdTypeInformationHelper - populates vtkSMIdTypeVectorProperty with the results of it's command
// .SECTION Description
// vtkSMSimpleIdTypeInformationHelper only works with 
// vtkSMIdTypeVectorProperties. It calls the property's Command on the
// root node of the associated server and populates the property using
// the values returned.
// .SECTION See Also
// vtkSMInformationHelper vtkSMIdTypeVectorProperty

#ifndef __vtkSMSimpleIdTypeInformationHelper_h
#define __vtkSMSimpleIdTypeInformationHelper_h

#include "vtkSMInformationHelper.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

class vtkSMProperty;

class VTK_EXPORT vtkSMSimpleIdTypeInformationHelper : 
  public vtkSMInformationHelper
{
public:
  static vtkSMSimpleIdTypeInformationHelper* New();
  vtkTypeMacro(vtkSMSimpleIdTypeInformationHelper, vtkSMInformationHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Updates the property using values obtained from the server. Calls
  // property's Command on the root node of the server and uses the
  // return value(s).
  virtual void UpdateProperty(vtkIdType connectionId, int serverIds, 
    vtkClientServerID objectId, vtkSMProperty* prop);
  //ETX

protected:
  vtkSMSimpleIdTypeInformationHelper();
  ~vtkSMSimpleIdTypeInformationHelper();

private:
  vtkSMSimpleIdTypeInformationHelper(const vtkSMSimpleIdTypeInformationHelper&); // Not implemented
  void operator=(const vtkSMSimpleIdTypeInformationHelper&); // Not implemented
};

#endif
