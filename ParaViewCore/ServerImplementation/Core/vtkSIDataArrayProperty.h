/*=========================================================================

  Program:   ParaView
  Module:    vtkSIDataArrayProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSIDataArrayProperty
 * @brief   InformationOnly property
 *
 * SIProperty that deals with vtkDataArray object type
*/

#ifndef vtkSIDataArrayProperty_h
#define vtkSIDataArrayProperty_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSIProperty.h"

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSIDataArrayProperty : public vtkSIProperty
{
public:
  static vtkSIDataArrayProperty* New();
  vtkTypeMacro(vtkSIDataArrayProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkSIDataArrayProperty();
  ~vtkSIDataArrayProperty();

  friend class vtkSIProxy;

  /**
   * Pull the current state of the underneath implementation
   */
  virtual bool Pull(vtkSMMessage*) VTK_OVERRIDE;

private:
  vtkSIDataArrayProperty(const vtkSIDataArrayProperty&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSIDataArrayProperty&) VTK_DELETE_FUNCTION;
};

#endif
