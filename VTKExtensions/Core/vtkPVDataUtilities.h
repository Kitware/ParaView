// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVDataUtilities
 * @brief data utility functions
 *
 * vtkPVDataUtilities is a collection of arbitrary data-specific utilities that
 * ParaView code can rely on.
 */

#ifndef vtkPVDataUtilities_h
#define vtkPVDataUtilities_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro

#include <string> // for std::string

class vtkDataObject;
class VTKPVVTKEXTENSIONSCORE_EXPORT vtkPVDataUtilities : public vtkObject
{
public:
  static vtkPVDataUtilities* New();
  vtkTypeMacro(vtkPVDataUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Given a composite dataset, ensure that all leaf datasets are assigned
   * names. Names are generated only they cannot be assigned based on
   * information already in the dataset. Once this function has been used to
   * assign names, use `GetAssignedNameForBlock` to access the name for each
   * leaf dataset.
   */
  static void AssignNamesToBlocks(vtkDataObject*);

  /**
   * Returns the name assigned for each leaf dataset. Only use after calling
   * `AssignNamesToBlocks`. `block` must be a non-null leaf dataset from the
   * composite dataset passed to `AssignNamesToBlocks`.
   */
  static std::string GetAssignedNameForBlock(vtkDataObject* block);

protected:
  vtkPVDataUtilities();
  ~vtkPVDataUtilities() override;

private:
  vtkPVDataUtilities(const vtkPVDataUtilities&) = delete;
  void operator=(const vtkPVDataUtilities&) = delete;
};

#endif
