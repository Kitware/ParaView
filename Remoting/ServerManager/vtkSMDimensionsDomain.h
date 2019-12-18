/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDimensionsDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMDimensionsDomain
 * @brief   int range domain based on the data dimensions.
 *
 * vtkSMDimensionsDomain is a subclass of vtkSMIntRangeDomain. It relies on two
 * required properties: "Input", "Direction". "Input" is generally an
 * vtkSMInputProperty which provides the information about the data extents.
 * "Direction" is an option required property which helps determine the
 * direction (VTK_XY_PLANE, VTK_YZ_PLANE or VTK_XZ_PLANE). If "Direction" is not
 * provided then the property must be a 3 element property while when Direction
 * is provided the property must be a 1 element property.
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
