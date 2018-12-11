/*=========================================================================

  Program:   ParaView
  Module:    vtkSITimeStepsProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSITimeRangeProperty
 *
 * SIProperty that deals with TimeRange on Algorithm object type
*/

#ifndef vtkSITimeStepsProperty_h
#define vtkSITimeStepsProperty_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSIProperty.h"

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSITimeStepsProperty : public vtkSIProperty
{
public:
  static vtkSITimeStepsProperty* New();
  vtkTypeMacro(vtkSITimeStepsProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSITimeStepsProperty();
  ~vtkSITimeStepsProperty() override;

  friend class vtkSIProxy;

  /**
   * Pull the current state of the underneath implementation
   */
  bool Pull(vtkSMMessage*) override;

private:
  vtkSITimeStepsProperty(const vtkSITimeStepsProperty&) = delete;
  void operator=(const vtkSITimeStepsProperty&) = delete;
};

#endif
