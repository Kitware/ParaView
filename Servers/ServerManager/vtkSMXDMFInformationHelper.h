/*=========================================================================

  Program:   ParaView
  Module:    vtkSMXDMFInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMXDMFInformationHelper - populates vtkSMStringVectorProperty using a vtkPVServerXDMFParameters
// .SECTION Description
// vtkSMXDMFInformationHelper only works with vtkSMStringVectorProperties. It
// populates the property using the server side helper object. Each
// XDMF parameters are stored as 5 component tuples: name, current value, 
// first index, stride, count
// .SECTION See Also
// vtkSMInformationHelper vtkPVServerXDMFParameters vtkSMStringVectorProperty

#ifndef __vtkSMXDMFInformationHelper_h
#define __vtkSMXDMFInformationHelper_h

#include "vtkSMInformationHelper.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

class vtkSMProperty;

class VTK_EXPORT vtkSMXDMFInformationHelper : public vtkSMInformationHelper
{
public:
  static vtkSMXDMFInformationHelper* New();
  vtkTypeRevisionMacro(vtkSMXDMFInformationHelper, vtkSMInformationHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Updates the property using value obtained for server. It creates
  // an instance of the server helper class vtkPVServerXDMFParameters
  // and passes the objectId (which the helper class gets as a pointer)
  // and populates the property using the values returned.
  // XDMF parameters are stored as 5 component tuples: name, current value, 
  // first index, stride, count
  virtual void UpdateProperty(
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop);
  //ETX

protected:
  vtkSMXDMFInformationHelper();
  ~vtkSMXDMFInformationHelper();

private:
  vtkSMXDMFInformationHelper(const vtkSMXDMFInformationHelper&); // Not implemented
  void operator=(const vtkSMXDMFInformationHelper&); // Not implemented
};

#endif
