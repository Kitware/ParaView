/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVInputProperty - Holds description of a VTK filters input.
// .SECTION Description
// This is a first attempt at separating a fitlers property from the
// user interface (in this case an input menu).
// Inputs are different from other properties in that they are stored
// in both vtkPVSource and vtkSource.  I may not need an input menu,
// but the input still needs to be set in batch scripts.

// Properties for UI (Properties Page) is an unfortunate KW naming convention.

#ifndef __vtkPVInputProperty_h
#define __vtkPVInputProperty_h


#include "vtkObject.h"
class vtkCollection;
class vtkPVDataSetAttributesInformation;
class vtkPVSource;

class VTK_EXPORT vtkPVInputProperty : public vtkObject
{
public:
  static vtkPVInputProperty* New();
  vtkTypeRevisionMacro(vtkPVInputProperty, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  void Copy(vtkPVInputProperty* in);

  // Description:
  // This method return 1 if the PVData matches the property.
  // The pvSource pointer is only used by one requirement so far.
  // vtkDataToDataSetFilters cannot change input types.
  int GetIsValidInput(vtkPVSource *input, vtkPVSource *pvs);

  // Description:
  // The name is used to construct methods for setting/adding/getting the input.
  // It is most commonly "Input", but can also be "Source" ...
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);
  
  // Description:
  // The type describes which describes the set on input classes
  // which the input will accept.  The value here is taken from
  // VTK definitions: VTK_DATA_SET, VTK_POINT_DATA, VTK_STRUCTURED_DATA,
  // VTK_POINT_SET, VTK_IMAGE_DATA, VTK_RECTILINEAR_GRID ...
  vtkSetStringMacro(Type);
  vtkGetStringMacro(Type);

protected:
  vtkPVInputProperty();
  ~vtkPVInputProperty();

  char* Name;
  char* Type;

  vtkPVInputProperty(const vtkPVInputProperty&); // Not implemented
  void operator=(const vtkPVInputProperty&); // Not implemented
};

#endif
