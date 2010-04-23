/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMInformationHelper - abstract superclass of all information helpers
// .SECTION Description
// Information helpers are used by information property to obtain
// information from the server. Since there can be more than way
// to populate an information property, this functionality was moved
// to a separate hierarchy of helper classes.
// Each information property has an associated information helper that
// is specified in the XML configuration file.
// .SECTION See Also
// vtkSMSimpleIntInformationHelper vtkSMSimpleDoubleInformationHelper 
// vtkSMSimpleStringInformationHelper

#ifndef __vtkSMInformationHelper_h
#define __vtkSMInformationHelper_h

#include "vtkSMObject.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

class vtkPVXMLElement;
class vtkSMProperty;

class VTK_EXPORT vtkSMInformationHelper : public vtkSMObject
{
public:
  vtkTypeMacro(vtkSMInformationHelper, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Fill in the property with values obtained from server. The
  // way in which the information is obtained depends on the sub-class.
  virtual void UpdateProperty(
    vtkIdType connectionId,
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop) = 0;
  //ETX

protected:
  vtkSMInformationHelper();
  ~vtkSMInformationHelper();

//BTX
  friend class vtkSMProperty;
//ETX

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty*, vtkPVXMLElement*) 
    {
      return 1;
    };

private:
  vtkSMInformationHelper(const vtkSMInformationHelper&); // Not implemented
  void operator=(const vtkSMInformationHelper&); // Not implemented
};

#endif
