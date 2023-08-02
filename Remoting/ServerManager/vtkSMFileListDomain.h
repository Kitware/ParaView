// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMFileListDomain
 * @brief   list of filenames
 *
 * vtkSMFileListDomain represents a domain consisting of a list of
 * filenames. It only works with vtkSMStringVectorProperty.
 */

#ifndef vtkSMFileListDomain_h
#define vtkSMFileListDomain_h

#include "vtkSMStringListDomain.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMFileListDomain : public vtkSMStringListDomain
{
public:
  static vtkSMFileListDomain* New();
  vtkTypeMacro(vtkSMFileListDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMFileListDomain();
  ~vtkSMFileListDomain() override;

private:
  vtkSMFileListDomain(const vtkSMFileListDomain&) = delete;
  void operator=(const vtkSMFileListDomain&) = delete;
};

#endif
