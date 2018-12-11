/*=========================================================================

  Program:   ParaView
  Module:    vtkSITimeRangeProperty.h

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

#ifndef vtkSITimeRangeProperty_h
#define vtkSITimeRangeProperty_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSIProperty.h"

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSITimeRangeProperty : public vtkSIProperty
{
public:
  static vtkSITimeRangeProperty* New();
  vtkTypeMacro(vtkSITimeRangeProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSITimeRangeProperty();
  ~vtkSITimeRangeProperty() override;

  friend class vtkSIProxy;

  /**
   * Pull the current state of the underneath implementation
   */
  bool Pull(vtkSMMessage*) override;

private:
  vtkSITimeRangeProperty(const vtkSITimeRangeProperty&) = delete;
  void operator=(const vtkSITimeRangeProperty&) = delete;
};

#endif
