// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVEnvironmentInformationHelper
 * @brief   Helper object that can
 * be used to obtain information about an environment.
 *
 * vtkPVEnvironmentInformationHelper can be used to get values of environment
 * variables.
 */

#ifndef vtkPVEnvironmentInformationHelper_h
#define vtkPVEnvironmentInformationHelper_h

#include "vtkObject.h"
#include "vtkRemotingCoreModule.h" //needed for exports

class VTKREMOTINGCORE_EXPORT vtkPVEnvironmentInformationHelper : public vtkObject
{
public:
  static vtkPVEnvironmentInformationHelper* New();
  vtkTypeMacro(vtkPVEnvironmentInformationHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the environment variable that we want to get the value of
   */
  vtkSetStringMacro(Variable);
  vtkGetStringMacro(Variable);
  ///@}

protected:
  vtkPVEnvironmentInformationHelper();
  ~vtkPVEnvironmentInformationHelper() override;

  char* Variable;

private:
  vtkPVEnvironmentInformationHelper(const vtkPVEnvironmentInformationHelper&) = delete;
  void operator=(const vtkPVEnvironmentInformationHelper&) = delete;
};

#endif
