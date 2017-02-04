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

#include "vtkPVServerManagerCoreModule.h" // For export macro
#include "vtkSMDomain.h"

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMIndexSelectionDomain : public vtkSMDomain
{
public:
  vtkTypeMacro(vtkSMIndexSelectionDomain, vtkSMDomain);
  virtual void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkSMIndexSelectionDomain* New();

  virtual int IsInDomain(vtkSMProperty* property) VTK_OVERRIDE;

  virtual int SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values) VTK_OVERRIDE;

  vtkSMProperty* GetInfoProperty() { return this->GetRequiredProperty("Info"); }

protected:
  vtkSMIndexSelectionDomain();
  ~vtkSMIndexSelectionDomain();

private:
  vtkSMIndexSelectionDomain(const vtkSMIndexSelectionDomain&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMIndexSelectionDomain&) VTK_DELETE_FUNCTION;
};

#endif // vtkSMIndexSelectionDomain_h
