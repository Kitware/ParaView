// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMDimensionsDomain
 * @brief   int range domain based on the data dimensions.
 *
 * vtkSMDimensionsDomain is a subclass of vtkSMIntRangeDomain. It relies on two
 * required properties: "Input", "Direction". "Input" is generally an
 * vtkSMInputProperty which provides the information about the data extents.
 * "Direction" is an option required property which helps determine the
 * direction (VTK_STRUCTURED_XY_PLANE, VTK_STRUCTURED_YZ_PLANE or VTK_STRUCTURED_XZ_PLANE).
 * If "Direction" is not provided then the property must be a 3 element property while when
 * Direction is provided the property must be a 1 element property.
 *
 * Supported Required-Property functions:
 * \li \c Input : points to a property that provides the data producer.
 * \li \c Direction: points to a property that provides the value for the
 *                   selected direction.
 */

#ifndef vtkSMDimensionsDomain_h
#define vtkSMDimensionsDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMIntRangeDomain.h"

class vtkSMIntVectorProperty;
class vtkSMProxyProperty;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMDimensionsDomain : public vtkSMIntRangeDomain
{
public:
  static vtkSMDimensionsDomain* New();
  vtkTypeMacro(vtkSMDimensionsDomain, vtkSMIntRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Update the domain using the "unchecked" values (if available) for all
   * required properties.
   */
  void Update(vtkSMProperty*) override;

protected:
  vtkSMDimensionsDomain();
  ~vtkSMDimensionsDomain() override;

  void Update(vtkSMProxyProperty* pp, vtkSMIntVectorProperty* ivp);
  int GetDirection(vtkSMIntVectorProperty* ivp);
  void GetExtent(vtkSMProxyProperty* pp, int extent[6]);

private:
  vtkSMDimensionsDomain(const vtkSMDimensionsDomain&) = delete;
  void operator=(const vtkSMDimensionsDomain&) = delete;
};

#endif
