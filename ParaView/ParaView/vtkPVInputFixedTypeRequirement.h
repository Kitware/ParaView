/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputFixedTypeRequirement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVInputFixedTypeRequirement - Type cannot change after input is set.
// .SECTION Description
// Used for vtkDataSetToDataSetFilter. Input type cannot change
// after it is set because the output will change type


    // Well I do not really like this hack, but it will work for DataSetToDataSetFilters.
    // We really need the old input, but I do not know which input it was.
    // Better would have been to passed the input property as an argument.
    // I already spent too long on this, so it will have to wait.

#ifndef __vtkPVInputFixedTypeRequirement_h
#define __vtkPVInputFixedTypeRequirement_h


class vtkDataSet;
class vtkPVData;
class vtkPVDataSetAttributesInformation;

#include "vtkPVInputRequirement.h"

class VTK_EXPORT vtkPVInputFixedTypeRequirement : public vtkPVInputRequirement
{
public:
  static vtkPVInputFixedTypeRequirement* New();
  vtkTypeRevisionMacro(vtkPVInputFixedTypeRequirement, vtkPVInputRequirement);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This method return 1 if the PVData matches the requirement.
  virtual int GetIsValidInput(vtkPVSource *input, vtkPVSource *pvs);

  // Description:
  // Called by vtkPVXMLPackageParser to configure the widget from XML
  // attributes.
  virtual int ReadXMLAttributes(vtkPVXMLElement* element,
                                vtkPVXMLPackageParser* parser);
  
protected:
  vtkPVInputFixedTypeRequirement();
  ~vtkPVInputFixedTypeRequirement() {};

  vtkPVInputFixedTypeRequirement(const vtkPVInputFixedTypeRequirement&); // Not implemented
  void operator=(const vtkPVInputFixedTypeRequirement&); // Not implemented
};

#endif
