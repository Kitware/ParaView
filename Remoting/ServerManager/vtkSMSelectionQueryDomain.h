/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelectionQueryDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMSelectionQueryDomain
 * @brief domain used for QueryString for selection source.
 *
 * vtkSMSelectionQueryDomain is used by "QueryString" property on a selection
 * source. It is primarily intended to fire domain modified event when the
 * selection query string's available options may have changed.
 */

#ifndef vtkSMSelectionQueryDomain_h
#define vtkSMSelectionQueryDomain_h

#include "vtkSMDomain.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMSelectionQueryDomain : public vtkSMDomain
{
public:
  static vtkSMSelectionQueryDomain* New();
  vtkTypeMacro(vtkSMSelectionQueryDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Update(vtkSMProperty* requestingProperty) override;

  /**
   * Returns currently chosen element type.
   */
  int GetElementType();

protected:
  vtkSMSelectionQueryDomain();
  ~vtkSMSelectionQueryDomain() override;

private:
  vtkSMSelectionQueryDomain(const vtkSMSelectionQueryDomain&) = delete;
  void operator=(const vtkSMSelectionQueryDomain&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
