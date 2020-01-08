/*=========================================================================

  Program:   ParaView
  Module:    vtkSMArrayRangeDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMArrayRangeDomain
 * @brief   double range domain based on array range
 *
 * vtkSMArrayRangeDomain is a sub-class of vtkSMDoubleRangeDomain. In it's
 * Update(), it sets min/max values based on the range of an input array.
 * It requires Input (vtkSMProxyProperty) and ArraySelection
 * (vtkSMStringVectorProperty) properties.
 * @sa
 * vtkSMDoubleRangeDomain vtkSMProxyProperty vtkSMStringVectorProperty
*/

#ifndef vtkSMArrayRangeDomain_h
#define vtkSMArrayRangeDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMDoubleRangeDomain.h"

class vtkSMSourceProxy;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMArrayRangeDomain : public vtkSMDoubleRangeDomain
{
public:
  static vtkSMArrayRangeDomain* New();
  vtkTypeMacro(vtkSMArrayRangeDomain, vtkSMDoubleRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Updates the range based on the scalar range of the currently selected
   * array. This requires Input (vtkSMProxyProperty) and ArraySelection
   * (vtkSMStringVectorProperty) properties. Currently, this uses
   * only the first component of the array.
   */
  void Update(vtkSMProperty* prop) override;

protected:
  vtkSMArrayRangeDomain();
  ~vtkSMArrayRangeDomain() override;

  void Update(const char* arrayName, int fieldAssociation, vtkSMSourceProxy* producer,
    int producerPort, int component = -1);

  friend class vtkSMBoundsDomain;
  friend class vtkSMRangedTransferFunctionDomain;

private:
  vtkSMArrayRangeDomain(const vtkSMArrayRangeDomain&) = delete;
  void operator=(const vtkSMArrayRangeDomain&) = delete;
};

#endif
