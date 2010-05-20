/*=========================================================================

  Program:   ParaView
  Module:    vtkSMArrayListInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMArrayListInformationHelper - populates
// vtkSMStringVectorProperty using a vtkSMArrayListDomain on the same property.
// .SECTION Description
// Unlike other informaition helpers, vtkSMArrayListInformationHelper does not 
// use server side objects directly. Instead it uses a vtkSMArrayListDomain
// on the information property itself to propulate the property. Look
// at XYPlotDisplay2 proxy for how to use this information helper.

#ifndef __vtkSMArrayListInformationHelper_h
#define __vtkSMArrayListInformationHelper_h

#include "vtkSMInformationHelper.h"

class VTK_EXPORT vtkSMArrayListInformationHelper : public vtkSMInformationHelper
{
public:
  static vtkSMArrayListInformationHelper* New();
  vtkTypeMacro(vtkSMArrayListInformationHelper, vtkSMInformationHelper);
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
  
  // Description:
  // Get/Set the name for the vtkSMArrayListDomain. If none is set,
  // the first vtkSMArrayListDomain in the property, if any, is used.
  // Can be set via xml using the "list_domain_name" attribute.
  vtkSetStringMacro(ListDomainName);
  vtkGetStringMacro(ListDomainName);

protected:
  vtkSMArrayListInformationHelper();
  ~vtkSMArrayListInformationHelper();

  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  char* ListDomainName;
private:
  vtkSMArrayListInformationHelper(const vtkSMArrayListInformationHelper&); // Not implemented.
  void operator=(const vtkSMArrayListInformationHelper&); // Not implemented.
};

#endif

