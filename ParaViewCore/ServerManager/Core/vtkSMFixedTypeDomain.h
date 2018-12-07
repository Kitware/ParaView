/*=========================================================================

  Program:   ParaView
  Module:    vtkSMFixedTypeDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMFixedTypeDomain
 * @brief   restricts the proxy to have the same type as previous proxy
 *
 * vtkSMFixedTypeDomain is used by input properties of filters that can
 * not have different input types after input is set the first time. For
 * example, a sub-class vtkDataSetToDataSetFilter, once connected in
 * a pipeline can not change it's input type, say, from vtkImageData to
 * vtkUnstructuredGrid because it's output can not change.
 * @sa
 * vtkSMDomain
*/

#ifndef vtkSMFixedTypeDomain_h
#define vtkSMFixedTypeDomain_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMDomain.h"

class vtkSMSourceProxy;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMFixedTypeDomain : public vtkSMDomain
{
public:
  static vtkSMFixedTypeDomain* New();
  vtkTypeMacro(vtkSMFixedTypeDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns true if the value of the property is in the domain.
   * The property has to be a vtkSMProxyProperty which points
   * to a vtkSMSourceProxy. If the new (unchecked) source proxy
   * has the same number of parts and data types as the old
   * (checked) one, it returns 1. Returns 0 otherwise.
   */
  int IsInDomain(vtkSMProperty* property) override;

  /**
   * Returns true if old and new source proxies have the same
   * output data type, false otherwise.
   */
  virtual int IsInDomain(vtkSMSourceProxy* oldProxy, vtkSMSourceProxy* newProxy);

protected:
  vtkSMFixedTypeDomain();
  ~vtkSMFixedTypeDomain() override;

private:
  vtkSMFixedTypeDomain(const vtkSMFixedTypeDomain&) = delete;
  void operator=(const vtkSMFixedTypeDomain&) = delete;
};

#endif
