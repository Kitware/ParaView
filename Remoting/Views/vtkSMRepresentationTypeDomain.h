/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRepresentationTypeDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMRepresentationTypeDomain
 * @brief   domain for "Representation" property on
 * representations in "RenderView".
 *
 * vtkSMRepresentationTypeDomain is designed to be used as the domain for
 * "Representation" property on representation proxies in the "RenderView". It
 * extends vtkSMStringListDomain to add logic to set default values based on the
 * input data information.
 *
 * Supported Required-Property functions:
 * \li \c Input : (optional) refers to a property that provides the data-producer.
 *                When present will be used to come up with default
 *                representation type using data information.
*/

#ifndef vtkSMRepresentationTypeDomain_h
#define vtkSMRepresentationTypeDomain_h

#include "vtkRemotingViewsModule.h" // needed for export macro.
#include "vtkSMStringListDomain.h"

class vtkPVDataInformation;

class VTKREMOTINGVIEWS_EXPORT vtkSMRepresentationTypeDomain : public vtkSMStringListDomain
{
public:
  static vtkSMRepresentationTypeDomain* New();
  vtkTypeMacro(vtkSMRepresentationTypeDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns 1 if the domain updated the property.
   * Overridden to use input data information to pick appropriate representation
   * type.
   */
  int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) override;

protected:
  vtkSMRepresentationTypeDomain();
  ~vtkSMRepresentationTypeDomain() override;

  /**
   * Returns the datainformation from the current input, if possible.
   */
  vtkPVDataInformation* GetInputInformation();

private:
  vtkSMRepresentationTypeDomain(const vtkSMRepresentationTypeDomain&) = delete;
  void operator=(const vtkSMRepresentationTypeDomain&) = delete;
};

#endif
