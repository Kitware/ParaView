/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputArrayRequirement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVInputArrayRequirement - Further restricts input by attributes.
// .SECTION Description
// Some filters should not accept inputs without specific attributes.
// An example is Contour requires point scalars.  This class holds
// the description of one attribute.  If you do not set one ivar,
// then it is not restricted:  i.e. Type not set means attribute
// can have any type.
// The vtkPVInput property is sort of an input restriction too 
// (excpet for name).  We might generalize and have boolean combiniations ... 

// Properties for UI (Properties Page) is an unfortunate KW naming convention.

#ifndef __vtkPVInputArrayRequirement_h
#define __vtkPVInputArrayRequirement_h


class vtkDataSet;
class vtkPVDataSetAttributesInformation;

#include "vtkPVInputRequirement.h"

class VTK_EXPORT vtkPVInputArrayRequirement : public vtkPVInputRequirement
{
public:
  static vtkPVInputArrayRequirement* New();
  vtkTypeRevisionMacro(vtkPVInputArrayRequirement, vtkPVInputRequirement);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This method return 1 if the PVData matches the property.
  virtual int GetIsValidInput(vtkPVSource* input, vtkPVSource* pvs);

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

  // Description:
  // Possible values are: 
  // vtkDataSet::POINT_DATA, vtkDataSet::CELL_DATA, vtkDataSet::DATA_OBJECT_FIELD
  // This method is only used for debugging.
  vtkGetMacro(Attribute,int);
  
  // Description:
  // Set the number of components.
  vtkSetMacro(NumberOfComponents,int);
  vtkGetMacro(NumberOfComponents,int);

  // Description:
  // Possible values are VTK_FLOAT, ....
  // This method is only used for debugging.
  vtkGetMacro(DataType,int);

protected:
  vtkPVInputArrayRequirement();
  ~vtkPVInputArrayRequirement() {};

  int Attribute;
  int DataType;
  int NumberOfComponents;

  int AttributeInfoContainsArray(vtkPVDataSetAttributesInformation* attrInfo);


  vtkPVInputArrayRequirement(const vtkPVInputArrayRequirement&); // Not implemented
  void operator=(const vtkPVInputArrayRequirement&); // Not implemented
};

#endif
