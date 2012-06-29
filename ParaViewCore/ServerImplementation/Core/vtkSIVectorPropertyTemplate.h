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
// .NAME vtkSIVectorPropertyTemplate
// .SECTION Description
//

#ifndef __vtkSIVectorPropertyTemplate_h
#define __vtkSIVectorPropertyTemplate_h

#include "vtkSIVectorProperty.h"

template <class T, class force_idtype=int>
class VTK_EXPORT vtkSIVectorPropertyTemplate : public vtkSIVectorProperty
{
public:
  typedef vtkSIVectorProperty Superclass;
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If ArgumentIsArray is true, multiple elements are passed in as
  // array arguments. For example, For example, if
  // RepeatCommand is true, NumberOfElementsPerCommand is 2, the
  // command is SetFoo and the values are 1 2 3 4 5 6, the resulting
  // stream will have:
  // @verbatim
  // * Invoke obj SetFoo array(1, 2)
  // * Invoke obj SetFoo array(3, 4)
  // * Invoke obj SetFoo array(5, 6)
  // @endverbatim
  vtkGetMacro(ArgumentIsArray, bool);

//BTX
protected:
  vtkSIVectorPropertyTemplate();
  ~vtkSIVectorPropertyTemplate();

  // Description:
  // Push a new state to the underneath implementation
  virtual bool Push(vtkSMMessage*, int);

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*);

  // Description:
  // Parse the xml for the property.
  virtual bool ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element);

  // Description:
  // Implements the actual push.
  bool Push(T* values, int number_of_elements);

  bool ArgumentIsArray;

private:
  vtkSIVectorPropertyTemplate(const vtkSIVectorPropertyTemplate&); // Not implemented
  void operator=(const vtkSIVectorPropertyTemplate&); // Not implemented
//ETX
};

#endif
