/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDataAssemblyListDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
 *      number_of_elements="1"
 *      default_values="Hierarchy">
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

protected:
  vtkSMDataAssemblyListDomain();
  ~vtkSMDataAssemblyListDomain();

private:
  vtkSMDataAssemblyListDomain(const vtkSMDataAssemblyListDomain&) = delete;
  void operator=(const vtkSMDataAssemblyListDomain&) = delete;
};

#endif
