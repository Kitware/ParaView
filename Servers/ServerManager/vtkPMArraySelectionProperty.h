/*=========================================================================

  Program:   ParaView
  Module:    vtkPMArraySelectionProperty

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPMArraySelectionProperty
// .SECTION Description
// PMProperty that deals with ArraySelection object

#ifndef __vtkPMArraySelectionProperty_h
#define __vtkPMArraySelectionProperty_h

#include "vtkPMProperty.h"
#include "vtkSMMessage.h"

class VTK_EXPORT vtkPMArraySelectionProperty : public vtkPMProperty
{
public:
  static vtkPMArraySelectionProperty* New();
  vtkTypeMacro(vtkPMArraySelectionProperty, vtkPMProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPMArraySelectionProperty();
  ~vtkPMArraySelectionProperty();

  friend class vtkPMProxy;

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*);

private:
  vtkPMArraySelectionProperty(const vtkPMArraySelectionProperty&); // Not implemented
  void operator=(const vtkPMArraySelectionProperty&); // Not implemented
//ETX
};

#endif
