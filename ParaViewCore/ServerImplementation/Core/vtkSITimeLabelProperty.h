/*=========================================================================

  Program:   ParaView
  Module:    vtkSITimeLabelProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSITimeLabelProperty
 *
 * SIProperty that deals with TimeLabel annotation on Algorithm object type
*/

#ifndef vtkSITimeLabelProperty_h
#define vtkSITimeLabelProperty_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSIProperty.h"

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSITimeLabelProperty : public vtkSIProperty
{
public:
  static vtkSITimeLabelProperty* New();
  vtkTypeMacro(vtkSITimeLabelProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkSITimeLabelProperty();
  ~vtkSITimeLabelProperty();

  friend class vtkSIProxy;

  /**
   * Pull the current state of the underneath implementation
   */
  virtual bool Pull(vtkSMMessage*) VTK_OVERRIDE;

private:
  vtkSITimeLabelProperty(const vtkSITimeLabelProperty&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSITimeLabelProperty&) VTK_DELETE_FUNCTION;
};

#endif
