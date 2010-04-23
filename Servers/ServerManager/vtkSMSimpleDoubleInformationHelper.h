/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSimpleDoubleInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSimpleDoubleInformationHelper - populates vtkSMDoubleVectorProperty with the results of it's command
// .SECTION Description
// vtkSMSimpleDoubleInformationHelper only works with 
// vtkSMDoubleVectorProperties. It calls the property's Command on the
// root node of the associated server and populates the property using
// the values returned.
// .SECTION See Also
// vtkSMInformationHelper vtkSMDoubleVectorProperty

#ifndef __vtkSMSimpleDoubleInformationHelper_h
#define __vtkSMSimpleDoubleInformationHelper_h

#include "vtkSMInformationHelper.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

class vtkSMProperty;

class VTK_EXPORT vtkSMSimpleDoubleInformationHelper : public vtkSMInformationHelper
{
public:
  static vtkSMSimpleDoubleInformationHelper* New();
  vtkTypeMacro(vtkSMSimpleDoubleInformationHelper, vtkSMInformationHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Updates the property using values obtained for server. Calls
  // property's Command on the root node of the server and uses the
  // return value(s).
  virtual void UpdateProperty(
    vtkIdType connectionId,
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop);
  //ETX

protected:
  vtkSMSimpleDoubleInformationHelper();
  ~vtkSMSimpleDoubleInformationHelper();

private:
  vtkSMSimpleDoubleInformationHelper(const vtkSMSimpleDoubleInformationHelper&); // Not implemented
  void operator=(const vtkSMSimpleDoubleInformationHelper&); // Not implemented
};

#endif
