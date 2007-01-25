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
// .NAME vtkSMXDMFInformationHelper - populates vtkSMStringVectorProperty with information from a vtkXdmfReader.
// .SECTION Description
// vtkSMXDMFInformationHelper only works with vtkSMStringVectorProperties. It
// populates the property using the server side helper object. 
// XDMF parameters are stored as 5 component tuples: name, current value, 
// first index, stride, count
// Domains and grids are returned in a simple list.
// .SECTION See Also
// vtkSMInformationHelper vtkPVServerXDMFParameters vtkSMStringVectorProperty

#ifndef __vtkSMXDMFInformationHelper_h
#define __vtkSMXDMFInformationHelper_h

#include "vtkSMInformationHelper.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

class vtkSMProperty;
class vtkPVXMLElement;

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
  virtual void UpdateProperty(vtkIdType connectionId,
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop);
//ETX

protected:
  vtkSMXDMFInformationHelper();
  ~vtkSMXDMFInformationHelper();

  //will look for the info_type attribute and if set to domains or grids
  //will act to gather them. otherwise it will gather parameters.
  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  int InfoType;
private:
  vtkSMXDMFInformationHelper(const vtkSMXDMFInformationHelper&); // Not implemented
  void operator=(const vtkSMXDMFInformationHelper&); // Not implemented
};

#endif
