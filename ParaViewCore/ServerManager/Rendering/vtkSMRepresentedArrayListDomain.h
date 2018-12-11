/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMRepresentedArrayListDomain
 * @brief   extends vtkSMArrayListDomain to add
 * support for arrays from represented data information.
 *
 * Representations often add new arrays on top of the ones provided by the
 * inputs to the representations. In that case, the domains for properties that
 * allow users to pick one of those newly added arrays need to show those
 * arrays e.g. "ColorArrayName" property of geometry representations. This
 * domain extends vtkSMArrayListDomain to add arrays from represented data
 * for representations.
*/

#ifndef vtkSMRepresentedArrayListDomain_h
#define vtkSMRepresentedArrayListDomain_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMArrayListDomain.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkSMRepresentationProxy;
class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMRepresentedArrayListDomain
  : public vtkSMArrayListDomain
{
public:
  static vtkSMRepresentedArrayListDomain* New();
  vtkTypeMacro(vtkSMRepresentedArrayListDomain, vtkSMArrayListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Update the domain.
   */
  void Update(vtkSMProperty*) override;

  //@{
  /**
   * Set this to true (default) to let this domain use the
   * represented-data-information from the outer  most representation proxy.
   * This ensures that for composite representations where user is provided with
   * a selection between multiple representations, the represented array list
   * domain fills its values up based on the currently active representation
   * type, rather than the representation subproxy from which the property was
   * exposed.
   * In XML, use 'use_true_parent' attribute on the domain element to set this
   * value.
   */
  vtkGetMacro(UseTrueParentForRepresentatedDataInformation, bool);
  //@}

protected:
  vtkSMRepresentedArrayListDomain();
  ~vtkSMRepresentedArrayListDomain() override;

  /**
   * Overridden to process "use_true_parent".
   */
  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* elem) override;

  /**
   * Returns true if an array should be filtered out based on its name or number
   * of tuples (for field data arrays).
   * This implementation returns true if the array name matches
   * an expression in the vtkPVColorArrayListSettings singleton.
   */
  bool IsFilteredArray(vtkPVDataInformation* info, int association, const char* arrayName) override;

  /**
   * HACK: Provides a temporary mechanism for subclasses to provide an
   * "additional" vtkPVDataInformation instance to get available arrays list
   * from.
   */
  vtkPVDataInformation* GetExtraDataInformation() override;

  void SetRepresentationProxy(vtkSMRepresentationProxy*);
  void OnRepresentationDataUpdated();

  vtkWeakPointer<vtkSMRepresentationProxy> RepresentationProxy;
  unsigned long ObserverId;

  vtkSetMacro(UseTrueParentForRepresentatedDataInformation, bool);
  bool UseTrueParentForRepresentatedDataInformation;

private:
  vtkSMRepresentedArrayListDomain(const vtkSMRepresentedArrayListDomain&) = delete;
  void operator=(const vtkSMRepresentedArrayListDomain&) = delete;
};

#endif
