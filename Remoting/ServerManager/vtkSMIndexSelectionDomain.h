/*==============================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMIndexSelectionDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

==============================================================================*/
/**
 * @class   vtkSMIndexSelectionDomain
 * @brief   Select names from an indexed string list.
 *
 *
 * See the vtkMPASReader proxy in readers.xml for how the properties should be
 * set up for this domain.
*/

#ifndef vtkSMIndexSelectionDomain_h
#define vtkSMIndexSelectionDomain_h

#include "vtkRemotingServerManagerModule.h" // For export macro
#include "vtkSMDomain.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMIndexSelectionDomain : public vtkSMDomain
{
public:
  vtkTypeMacro(vtkSMIndexSelectionDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkSMIndexSelectionDomain* New();

  int IsInDomain(vtkSMProperty* property) override;

  int SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values) override;

  vtkSMProperty* GetInfoProperty() { return this->GetRequiredProperty("Info"); }

protected:
  vtkSMIndexSelectionDomain();
  ~vtkSMIndexSelectionDomain() override;

private:
  vtkSMIndexSelectionDomain(const vtkSMIndexSelectionDomain&) = delete;
  void operator=(const vtkSMIndexSelectionDomain&) = delete;
};

#endif // vtkSMIndexSelectionDomain_h
