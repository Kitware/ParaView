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
/**
 * @class   vtkSIIndexSelectionProperty
 * @brief   Select names from an indexed string list.
 *
 * Expected Methods on reader (assuming command="Dimension"):
 * int GetNumberOfDimensions()
 * std::string GetDimensionName(int)
 * int GetCurrentDimensionIndex(std::string)
 * int GetDimensionSize(std::string)
 * void SetCurrentDimensionIndex(std::string, int)
*/

#ifndef vtkSIIndexSelectionProperty_h
#define vtkSIIndexSelectionProperty_h

#include "vtkPVServerImplementationCoreModule.h" // needed for exports
#include "vtkSIProperty.h"

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSIIndexSelectionProperty : public vtkSIProperty
{
public:
  static vtkSIIndexSelectionProperty* New();
  vtkTypeMacro(vtkSIIndexSelectionProperty, vtkSIProperty) void PrintSelf(
    ostream& os, vtkIndent indent) override;

protected:
  vtkSIIndexSelectionProperty();
  ~vtkSIIndexSelectionProperty() override;

  friend class vtkSIProxy;

  /**
   * Pull the current state of the underneath implementation
   */
  bool Pull(vtkSMMessage*) override;

private:
  vtkSIIndexSelectionProperty(const vtkSIIndexSelectionProperty&) = delete;
  void operator=(const vtkSIIndexSelectionProperty&) = delete;
};

#endif
