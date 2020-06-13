/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDataAssemblyDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMDataAssemblyDomain
 * @brief a domain that uses vtkDataAssembly
 *
 * vtkSMDataAssemblyDomain is intended for vtkSMStringVectorProperty that uses
 * vtkDataAssembly to qualify values. For example, the "Paths" property on
 * `vtkExtractBlockUsingDataAssembly` filter that lets the user select paths to
 * extract uses this domain.
 *
 * Example usage in a ServerManager XML is as follows:
 *
 * @code{xml}
 * <SourceProxy ...>
 * ...
 *   <StringVectorProperty clean_command="ClearNodePaths"
 *                      command="AddNodePath"
 *                      name="Paths"
 *                      number_of_elements_per_command="1"
 *                      panel_visibility="default"
 *                      repeat_command="1"
 *                      animateable="0">
 *     <DataAssemblyDomain name="data_assembly">
 *       <RequiredProperties>
 *         <Property function="Input" name="Input" />
 *       </RequiredProperties>
 *     </DataAssemblyDomain>
 *     <Documentation>
 *       This property lists the paths for blocks to extract.
 *     </Documentation>
 *   </StringVectorProperty>
 * </SourceProxy>
 * @endcode
 *
 */

#ifndef vtkSMDataAssemblyDomain_h
#define vtkSMDataAssemblyDomain_h

#include "vtkRemotingServerManagerModule.h" // needed for exports
#include "vtkSMDomain.h"
#include "vtkSmartPointer.h" //  needed for vtkSmartPointer
class vtkDataAssembly;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMDataAssemblyDomain : public vtkSMDomain
{
public:
  static vtkSMDataAssemblyDomain* New();
  vtkTypeMacro(vtkSMDataAssemblyDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Provides access to the current data assembly.
   */
  vtkDataAssembly* GetDataAssembly();

  void Update(vtkSMProperty* requestingProperty) override;

protected:
  vtkSMDataAssemblyDomain();
  ~vtkSMDataAssemblyDomain();

private:
  vtkSMDataAssemblyDomain(const vtkSMDataAssemblyDomain&) = delete;
  void operator=(const vtkSMDataAssemblyDomain&) = delete;

  bool DataAssemblyValid;
  vtkSmartPointer<vtkDataAssembly> DataAssembly;
};

#endif
