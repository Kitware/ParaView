// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
