/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputGroupRequirement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVInputGroupRequirement - Further restricts input by group.
// .SECTION Description
// Some filters should only operate on inputs with groups of objects
// (i.e. Extract part).  Others accept only inputs with a single part.

#ifndef __vtkPVInputGroupRequirement_h
#define __vtkPVInputGroupRequirement_h


class vtkDataSet;
class vtkPVData;
class vtkPVDataSetAttributesInformation;

#include "vtkPVInputRequirement.h"

class VTK_EXPORT vtkPVInputGroupRequirement : public vtkPVInputRequirement
{
public:
  static vtkPVInputGroupRequirement* New();
  vtkTypeRevisionMacro(vtkPVInputGroupRequirement, vtkPVInputRequirement);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This method return 1 if the PVData matches the property.
  virtual int GetIsValidInput(vtkPVSource *input, vtkPVSource *pvs);


  // Description:
  // Called by vtkPVXMLPackageParser to configure the widget from XML
  // attributes.
  virtual int ReadXMLAttributes(vtkPVXMLElement* element,
                                vtkPVXMLPackageParser* parser);

  // Description:
  // Possible values are: 
  // Positive value means exactly that many parts.
  // Value -1 means more than one part.
  vtkGetMacro(Quantity,int);
  
protected:
  vtkPVInputGroupRequirement();
  ~vtkPVInputGroupRequirement() {};

  int Quantity;

  vtkPVInputGroupRequirement(const vtkPVInputGroupRequirement&); // Not implemented
  void operator=(const vtkPVInputGroupRequirement&); // Not implemented
};

#endif
