/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSubsetInclusionLatticeDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMSubsetInclusionLatticeDomain
 * @brief domain for block selection properties using a
 * vtkSubsetInclusionLattice.
 *
 * vtkSMSubsetInclusionLatticeDomain is the domain to use for a
 * vtkSMStringVectorProperty permitting uses to select blocks with structure
 * described using vtkSubsetInclusionLattice or subclass.
 *
 * vtkSMSubsetInclusionLatticeDomain replaces vtkSMSILDomain. vtkSMSILDomain is
 * available for old-style readers that have not been updated to produce
 * vtkSubsetInclusionLattice and instead simply generate a vtkGraph for
 * representing the SIL.
 *
 * @section ExampleServerManagerXML Example Server-Manager configuration XML
 *
 * @code{xml}
 *  <StringVectorProperty ...>
 *    <SubsetInclusionLatticeDomain name="sil" class="vtkCGNSSubsetInclusionLattice"
 * default_path="//Grid">
 *      <RequiredProperties>
 *          <Property name="SILUpdateStamp" function="TimeStamp" />
 *      </RequiredProperties>
 *    </SubsetInclusionLatticeDomain>
 *  </StringVectorProperty>
 * @endcode
 *
 * @section SupportedAttributes Supported Attributes
 *
 * vtkSMSubsetInclusionLatticeDomain supports that following attributes that can
 * be specified in the XML configuration for the domain.
 *
 * @subsection ClassAttribute class attribute
 *
 * vtkSubsetInclusionLattice is the default, if not specified. However readers
 * often create subclasses for vtkSubsetInclusionLattice that make it easier to
 * identify nodes in the SIL using native terminology for the file format e.g.
 * `vtkCGNSSubsetInclusionLattice`. To indicate to the domain to create a
 * specific subclass, one can use the `class` attribute.
 *
 * @subsection DefaultPathAttribute default_path attribute
 *
 * `default_path` attribute can be set to a string that defines a path to use to
 * select nodes by default. If not empty, same as calling
 * `vtkSubsetInclusionLattice::SelectAll` during initialization to pick default
 * block selection state.
 *
 * @section RequiredProperties Supported required properties.
 *
 * vtkSubsetInclusionLattice depends on a required property with function `TimeStamp`
 * which is an `information_only` property of type `vtkSMIdTypeVectorProperty`. This
 * property indicates a time-stamp for when the SIL was rebuilt by the reader.
 * This is used to limit fetching of SIL from the server only when it has
 * changed.
 *
 * @sa vtkSMSILDomain
 */

#ifndef vtkSMSubsetInclusionLatticeDomain_h
#define vtkSMSubsetInclusionLatticeDomain_h

#include "vtkSMStringListDomain.h"

#include "vtkSmartPointer.h" // for vtkSmartPointer.
#include <string>            // for std::string

class vtkSubsetInclusionLattice;
class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMSubsetInclusionLatticeDomain
  : public vtkSMStringListDomain
{
public:
  static vtkSMSubsetInclusionLatticeDomain* New();
  vtkTypeMacro(vtkSMSubsetInclusionLatticeDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns 1 always. vtkSMSubsetInclusionLatticeDomain doesn't validate
   * values currently.
   */
  int IsInDomain(vtkSMProperty*) override { return 1; }

  /**
   * The SIL may have changed. Update it.
   */
  void Update(vtkSMProperty* requestingProperty) override;

  /**
   * Returns the vtkSubsetInclusionLattice. May return an empty SIL, but never a
   * nullptr.
   */
  vtkSubsetInclusionLattice* GetSIL();

  /**
   * Overridden to set default from SIL.
   */
  int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) override;

protected:
  vtkSMSubsetInclusionLatticeDomain();
  ~vtkSMSubsetInclusionLatticeDomain();

  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* elem) override;

private:
  vtkSMSubsetInclusionLatticeDomain(const vtkSMSubsetInclusionLatticeDomain&) = delete;
  void operator=(const vtkSMSubsetInclusionLatticeDomain&) = delete;

  std::string DefaultPath;
  vtkSmartPointer<vtkSubsetInclusionLattice> SIL;
  vtkIdType TimeTag;
};

#endif
