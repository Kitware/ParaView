/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExtentDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMExtentDomain
 * @brief   int range domain based on data set extent
 *
 * vtkSMExtentDomain is a subclass of vtkSMIntRangeDomain. In its Update
 * method, it determines the minimum and maximum extent in each dimension of
 * the structured data set with which it is associated. It requires a
 * vtkSMSourceProxy to do this.
 * @sa
 * vtkSMIntRangeDomain
*/

#ifndef vtkSMExtentDomain_h
#define vtkSMExtentDomain_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMIntRangeDomain.h"

class vtkSMProxyProperty;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMExtentDomain : public vtkSMIntRangeDomain
{
public:
  static vtkSMExtentDomain* New();
  vtkTypeMacro(vtkSMExtentDomain, vtkSMIntRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Update self checking the "unchecked" values of all required
   * properties. Overwritten by sub-classes.
   */
  virtual void Update(vtkSMProperty*) VTK_OVERRIDE;

  /**
   * Set the value of an element of a property from the animation editor.
   */
  virtual void SetAnimationValue(vtkSMProperty* property, int idx, double value) VTK_OVERRIDE;

  //@{
  /**
   * A vtkSMProperty is often defined with a default value in the
   * XML itself. However, many times, the default value must be determined
   * at run time. To facilitate this, domains can override this method
   * to compute and set the default value for the property.
   * Note that unlike the compile-time default values, the
   * application must explicitly call this method to initialize the
   * property.
   */
  virtual int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) VTK_OVERRIDE;

protected:
  vtkSMExtentDomain();
  ~vtkSMExtentDomain();
  //@}

  void Update(vtkSMProxyProperty* pp);

private:
  vtkSMExtentDomain(const vtkSMExtentDomain&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMExtentDomain&) VTK_DELETE_FUNCTION;
};

#endif
