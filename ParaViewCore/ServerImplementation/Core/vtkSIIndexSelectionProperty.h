/*=========================================================================

  Program:   ParaView
  Module:    vtkSIIndexSelectionProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIIndexSelectionProperty - Select names from an indexed string list.
// .SECTION Description
// Expected Methods on reader (assuming command="Dimension"):
// int GetNumberOfDimensions()
// std::string GetDimensionName(int)
// int GetCurrentDimensionIndex(std::string)
// int GetDimensionSize(std::string)
// void SetCurrentDimensionIndex(std::string, int)

#ifndef vtkSIIndexSelectionProperty_h
#define vtkSIIndexSelectionProperty_h

#include "vtkPVServerImplementationCoreModule.h" // needed for exports
#include "vtkSIProperty.h"

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSIIndexSelectionProperty :
    public vtkSIProperty
{
public:
  static vtkSIIndexSelectionProperty* New();
  vtkTypeMacro(vtkSIIndexSelectionProperty, vtkSIProperty)
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSIIndexSelectionProperty();
  ~vtkSIIndexSelectionProperty();

  friend class vtkSIProxy;

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*);

private:
  vtkSIIndexSelectionProperty(const vtkSIIndexSelectionProperty&); // Not implemented
  void operator=(const vtkSIIndexSelectionProperty&); // Not implemented
//ETX
};

#endif
