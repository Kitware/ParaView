/*=========================================================================

  Program:   earaView
  Module:    vtkPEquivalenceSet.h

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  Copyright 2013 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.

=========================================================================*/
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
