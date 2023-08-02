// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright 2013 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkPEquivalenceSet
 * @brief   distributed method of Equivalence
 *
 * Same as EquivalenceSet, but resolving is a global operation.
 * .SEE vtkEquivalenceSet
 */

#ifndef vtkPEquivalenceSet_h
#define vtkPEquivalenceSet_h

#include "vtkEquivalenceSet.h"
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPEquivalenceSet : public vtkEquivalenceSet
{
public:
  vtkTypeMacro(vtkPEquivalenceSet, vtkEquivalenceSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPEquivalenceSet* New();

  // Globally equivalent set IDs are reassigned to be sequential.
  int ResolveEquivalences() override;

protected:
  vtkPEquivalenceSet();
  ~vtkPEquivalenceSet() override;

private:
  vtkPEquivalenceSet(const vtkPEquivalenceSet&) = delete;
  void operator=(const vtkPEquivalenceSet&) = delete;
};

#endif /* vtkPEquivalenceSet_h */
