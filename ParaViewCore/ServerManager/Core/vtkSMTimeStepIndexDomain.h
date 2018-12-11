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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Update self checking the "unchecked" values of all required
   * properties. Overwritten by sub-classes.
   */
  void Update(vtkSMProperty*) override;

protected:
  vtkSMTimeStepIndexDomain();
  ~vtkSMTimeStepIndexDomain() override;

  void Update(vtkSMProxyProperty* pp);

private:
  vtkSMTimeStepIndexDomain(const vtkSMTimeStepIndexDomain&) = delete;
  void operator=(const vtkSMTimeStepIndexDomain&) = delete;
};

#endif // vtkSMTimeStepIndexDomain_h
