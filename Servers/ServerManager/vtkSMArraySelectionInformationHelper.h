/*=========================================================================

  Program:   ParaView
  Module:    vtkSMArraySelectionInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMArraySelectionInformationHelper - populates 
// vtkSMStringVectorProperty using a vtkPVServerArraySelection
// .SECTION Description
// vtkSMArraySelectionInformationHelper only works with 
// vtkSMStringVectorProperties. It populates the property using the server 
// side helper object. Each array is represented by two components:
// name, state (on/off)
// .SECTION See Also
// vtkSMInformationHelper vtkPVServerArraySelection vtkSMStringVectorProperty

#ifndef __vtkSMArraySelectionInformationHelper_h
#define __vtkSMArraySelectionInformationHelper_h

#include "vtkSMInformationHelper.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

class vtkSMProperty;

class VTK_EXPORT vtkSMArraySelectionInformationHelper : public vtkSMInformationHelper
{
public:
  static vtkSMArraySelectionInformationHelper* New();
  vtkTypeMacro(vtkSMArraySelectionInformationHelper, vtkSMInformationHelper);
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
  vtkSMArraySelectionInformationHelper();
  ~vtkSMArraySelectionInformationHelper();

  char* AttributeName;

  vtkSetStringMacro(AttributeName);
  vtkGetStringMacro(AttributeName);

  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

private:
  vtkSMArraySelectionInformationHelper(const vtkSMArraySelectionInformationHelper&); // Not implemented
  void operator=(const vtkSMArraySelectionInformationHelper&); // Not implemented
};

#endif
