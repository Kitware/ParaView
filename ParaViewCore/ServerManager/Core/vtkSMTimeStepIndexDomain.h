/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTimeStepIndexDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMTimeStepIndexDomain
 * @brief   int range domain based on data set time-steps
 *
 * vtkSMTimeStepIndexDomain is a subclass of vtkSMIntRangeDomain. In its Update
 * method, it determines the number of time steps in the associated data.
 * It requires a vtkSMSourceProxy to do this.
 * @sa
 * vtkSMIntRangeDomain
*/

#ifndef vtkSMTimeStepIndexDomain_h
#define vtkSMTimeStepIndexDomain_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMIntRangeDomain.h"

class vtkSMProxyProperty;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMTimeStepIndexDomain : public vtkSMIntRangeDomain
{
public:
  static vtkSMTimeStepIndexDomain* New();
  vtkTypeMacro(vtkSMTimeStepIndexDomain, vtkSMIntRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Update self checking the "unchecked" values of all required
   * properties. Overwritten by sub-classes.
   */
  virtual void Update(vtkSMProperty*) VTK_OVERRIDE;

protected:
  vtkSMTimeStepIndexDomain();
  ~vtkSMTimeStepIndexDomain();

  void Update(vtkSMProxyProperty* pp);

private:
  vtkSMTimeStepIndexDomain(const vtkSMTimeStepIndexDomain&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMTimeStepIndexDomain&) VTK_DELETE_FUNCTION;
};

#endif // vtkSMTimeStepIndexDomain_h
