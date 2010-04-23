/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStringArrayHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMStringArrayHelper - Helper to get values of a string array
// .SECTION Description
// This is a helper class to deliver the contents of a vtkStringArray
// during UpdatePropertyInformation(). It fills a vtkSMStringVectorProperty
// with the values of the string array returned by the command defined
// by the property.

#ifndef __vtkSMStringArrayHelper_h
#define __vtkSMStringArrayHelper_h

#include "vtkSMInformationHelper.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

class vtkSMProperty;

class VTK_EXPORT vtkSMStringArrayHelper : public vtkSMInformationHelper
{
public:
  static vtkSMStringArrayHelper* New();
  vtkTypeMacro(vtkSMStringArrayHelper, vtkSMInformationHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Updates the property using value obtained for server.
  virtual void UpdateProperty(
    vtkIdType connectionId,
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop);
  //ETX

protected:
  vtkSMStringArrayHelper();
  ~vtkSMStringArrayHelper();

private:
  vtkSMStringArrayHelper(const vtkSMStringArrayHelper&); // Not implemented
  void operator=(const vtkSMStringArrayHelper&); // Not implemented
};

#endif
