/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputRequirement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVInputRequirement - Restrict allowable input.
// .SECTION Description
// Some filters should not accept inputs without specific attributes.
// An example is Contour requires point scalars.  
// This is a supperclass for objects that describe input requirments.
// New subclasses can be added (and created through XML) for any
// crazy restriction.

#ifndef __vtkPVInputRequirement_h
#define __vtkPVInputRequirement_h

class vtkPVDataSetAttributesInformation;
class vtkPVSource;
class vtkPVXMLElement;
class vtkPVXMLPackageParser;

#include "vtkObject.h"

class VTK_EXPORT vtkPVInputRequirement : public vtkObject
{
public:
  static vtkPVInputRequirement* New();
  vtkTypeRevisionMacro(vtkPVInputRequirement, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method return 1 if the PVData matches the requirement.
  // The pvSource pointer is only used by one requirement so far.
  // vtkDataToDataSetFilters cannot change input types.
  virtual int GetIsValidInput(vtkPVSource *input, vtkPVSource *pvs);

  // Description:
  // This are used by the field menu to determine is a field
  // should be selectable.
  virtual int GetIsValidField(int field, 
                              vtkPVDataSetAttributesInformation* info);

  // Description:
  // Called by vtkPVXMLPackageParser to configure the widget from XML
  // attributes.
  virtual int ReadXMLAttributes(vtkPVXMLElement* element,
                                vtkPVXMLPackageParser* parser);

protected:
  vtkPVInputRequirement() {};
  ~vtkPVInputRequirement() {};

  vtkPVInputRequirement(const vtkPVInputRequirement&); // Not implemented
  void operator=(const vtkPVInputRequirement&); // Not implemented
};

#endif
