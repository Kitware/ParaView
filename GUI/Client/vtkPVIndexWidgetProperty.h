/*=========================================================================

  Program:   ParaView
  Module:    vtkPVIndexWidgetProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVIndexWidgetProperty
// .SECTION Description

#ifndef __vtkPVIndexWidgetProperty_h
#define __vtkPVIndexWidgetProperty_h

#include "vtkPVWidgetProperty.h"

class VTK_EXPORT vtkPVIndexWidgetProperty : public vtkPVWidgetProperty
{
public:
  static vtkPVIndexWidgetProperty* New();
  vtkTypeRevisionMacro(vtkPVIndexWidgetProperty, vtkPVWidgetProperty);
  void PrintSelf(ostream &os, vtkIndent indent);
  
  vtkSetMacro(Index, int);
  vtkGetMacro(Index, int);
  
  vtkSetStringMacro(VTKCommand);
  
  void AcceptInternal();
  
protected:
  vtkPVIndexWidgetProperty();
  ~vtkPVIndexWidgetProperty();
  
  int Index;
  char *VTKCommand;
  
private:
  vtkPVIndexWidgetProperty(const vtkPVIndexWidgetProperty&); // Not implemented
  void operator=(const vtkPVIndexWidgetProperty&); // Not implemented
};

#endif
