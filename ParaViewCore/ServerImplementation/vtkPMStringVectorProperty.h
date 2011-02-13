/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPMStringVectorProperty
// .SECTION Description
//

#ifndef __vtkPMStringVectorProperty_h
#define __vtkPMStringVectorProperty_h

#include "vtkPMVectorProperty.h"

//BTX
#include <vtkstd/vector>
#include <vtkstd/string>
//ETX

class VTK_EXPORT vtkPMStringVectorProperty : public vtkPMVectorProperty
{
public:
  static vtkPMStringVectorProperty* New();
  vtkTypeMacro(vtkPMStringVectorProperty, vtkPMVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPMStringVectorProperty();
  ~vtkPMStringVectorProperty();

  enum ElementTypes{ INT, DOUBLE, STRING };

  // Description:
  // Push a new state to the underneath implementation
  virtual bool Push(vtkSMMessage*, int);

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*);

  // Description:
  // Parse the xml for the property.
  virtual bool ReadXMLAttributes(vtkPMProxy* proxyhelper, vtkPVXMLElement* element);

private:
  vtkPMStringVectorProperty(const vtkPMStringVectorProperty&); // Not implemented
  void operator=(const vtkPMStringVectorProperty&); // Not implemented

  bool Push(const vtkstd::vector<vtkstd::string> &values);
  vtkstd::vector<int> ElementTypes;
//ETX
};

#endif
