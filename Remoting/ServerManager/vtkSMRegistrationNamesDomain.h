/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRegistrationNamesDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMRegistrationNamesDomain
 * @brief domain to set default value for a property using proxy registration names
 *
 * vtkSMRegistrationNamesDomain can be used for string-vector property to initialize it
 * with registration names for proxies in the required property named "Proxies".
 *
 * For example
 * @code{xml}
 *  <SourceProxy name="GroupDataSets" ...>
 *    <InputProperty clean_command="RemoveAllInputs"
 *                   command="AddInputConnection"
 *                   multiple_input="1"
 *                   name="Input">
 *                   ...
 *    </InputProperty>
 *
 *    <StringVectorProperty name="BlockNames"
 *      command="SetInputName"
 *      number_of_elements_per_command="1"
 *      repeat_command="1"
 *      use_index="1"
 *      clean_command="ClearInputNames">
 *      <Documentation>
 *        Specify names to use for each input.
 *      </Documentation>
 *      <RegistrationNamesDomain name="names_list" registration_group="sources">
 *        <RequiredProperties>
 *          <Property name="Input" function="Proxies" />
 *        </RequiredProperties>
 *      </RegistrationNamesDomain>
 *    </StringVectorProperty>

 *    ....
 *  </SourceProxy>
 * @endcode
 */

#ifndef vtkSMRegistrationNamesDomain_h
#define vtkSMRegistrationNamesDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMDomain.h"
#include <memory> // for std::unique_ptr

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMRegistrationNamesDomain : public vtkSMDomain
{
public:
  static vtkSMRegistrationNamesDomain* New();
  vtkTypeMacro(vtkSMRegistrationNamesDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) override;

protected:
  vtkSMRegistrationNamesDomain();
  ~vtkSMRegistrationNamesDomain() override;

  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

private:
  vtkSMRegistrationNamesDomain(const vtkSMRegistrationNamesDomain&) = delete;
  void operator=(const vtkSMRegistrationNamesDomain&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
