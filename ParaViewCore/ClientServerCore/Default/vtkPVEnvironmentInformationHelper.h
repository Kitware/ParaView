/*=========================================================================

  Program:   ParaView
  Module:    vtkPVEnvironmentInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
#include "vtkPVClientServerCoreDefaultModule.h" //needed for exports

class VTKPVCLIENTSERVERCOREDEFAULT_EXPORT vtkPVEnvironmentInformationHelper : public vtkObject
{
public:
  static vtkPVEnvironmentInformationHelper* New();
  vtkTypeMacro(vtkPVEnvironmentInformationHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the environment variable that we want to get the value of
   */
  vtkSetStringMacro(Variable);
  vtkGetStringMacro(Variable);
  //@}

protected:
  vtkPVEnvironmentInformationHelper();
  ~vtkPVEnvironmentInformationHelper() override;

  char* Variable;

private:
  vtkPVEnvironmentInformationHelper(const vtkPVEnvironmentInformationHelper&) = delete;
  void operator=(const vtkPVEnvironmentInformationHelper&) = delete;
};

#endif
