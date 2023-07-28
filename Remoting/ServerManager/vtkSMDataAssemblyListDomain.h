// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSMDataAssemblyListDomain
 * @brief domain listing available assembly types in the input
 *
 * vtkSMDataAssemblyListDomain is a string-list domain subclass that populates
 * the list with names of assemblies available in the input.
 *
 * Example usage is as follows:
 *
 * @code{xml}
 * <SourceProxy ...>
 *   <StringVectorProperty name="BlockAssembly"
 *      command="SetSelectorAssembly"
 *      number_of_elements="1">
 *      <DataAssemblyListDomain name="data_assembly_list">
 *        <RequiredProperties>
 *          <Property function="Input" name="Input" />
 *        </RequiredProperties>
 *      </DataAssemblyListDomain>
 *    </StringVectorProperty>
 * </SourceProxy>
 * @endcode
 */

#ifndef vtkSMDataAssemblyListDomain_h
#define vtkSMDataAssemblyListDomain_h

#include "vtkRemotingServerManagerModule.h" // needed for exports
#include "vtkSMStringListDomain.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMDataAssemblyListDomain : public vtkSMStringListDomain
{
public:
  static vtkSMDataAssemblyListDomain* New();
  vtkTypeMacro(vtkSMDataAssemblyListDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Update(vtkSMProperty* requestingProperty) override;

  int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) override;

  ///@{
  /**
   * When set to true, the domain will use the old functionality setting the
   * assembly default value. This is useful for backwards compatibility.
   */
  vtkSetMacro(BackwardCompatibilityMode, bool);
  vtkGetMacro(BackwardCompatibilityMode, bool);
  vtkBooleanMacro(BackwardCompatibilityMode, bool);
  ///@}

protected:
  vtkSMDataAssemblyListDomain();
  ~vtkSMDataAssemblyListDomain() override;

  bool BackwardCompatibilityMode = false;

private:
  vtkSMDataAssemblyListDomain(const vtkSMDataAssemblyListDomain&) = delete;
  void operator=(const vtkSMDataAssemblyListDomain&) = delete;
};

#endif
