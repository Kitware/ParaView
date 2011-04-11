/*=========================================================================

  Program:   ParaView
  Module:    vtkSIArraySelectionProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIArraySelectionProperty - InformationOnly property
// .SECTION Description
// SIProperty that deals with ArraySelection object

#ifndef __vtkSIArraySelectionProperty_h
#define __vtkSIArraySelectionProperty_h

#include "vtkSIProperty.h"

class VTK_EXPORT vtkSIArraySelectionProperty : public vtkSIProperty
{
public:
  static vtkSIArraySelectionProperty* New();
  vtkTypeMacro(vtkSIArraySelectionProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSIArraySelectionProperty();
  ~vtkSIArraySelectionProperty();

  friend class vtkSIProxy;

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*);

private:
  vtkSIArraySelectionProperty(const vtkSIArraySelectionProperty&); // Not implemented
  void operator=(const vtkSIArraySelectionProperty&); // Not implemented
//ETX
};

#endif
