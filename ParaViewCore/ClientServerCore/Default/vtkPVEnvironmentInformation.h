/*=========================================================================

  Program:   ParaView
  Module:    vtkPVEnvironmentInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVEnvironmentInformation
 * @brief   Information object that can
 * be used to obtain values of environment variables.
 *
 * vtkPVEnvironmentInformation can be used to get values of environment
 * variables.
*/

#ifndef vtkPVEnvironmentInformation_h
#define vtkPVEnvironmentInformation_h

#include "vtkPVClientServerCoreDefaultModule.h" //needed for exports
#include "vtkPVInformation.h"

class VTKPVCLIENTSERVERCOREDEFAULT_EXPORT vtkPVEnvironmentInformation : public vtkPVInformation
{
public:
  static vtkPVEnvironmentInformation* New();
  vtkTypeMacro(vtkPVEnvironmentInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Transfer information about a single object into this object.
   * The object must be a vtkPVEnvironmentInformationHelper.
   */
  void CopyFromObject(vtkObject* object) override;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  //@}

  //@{
  /**
   * Get the value of an environment variable
   */
  vtkGetStringMacro(Variable);
  //@}

protected:
  vtkPVEnvironmentInformation();
  ~vtkPVEnvironmentInformation() override;

  char* Variable; // value of an environment variable

  vtkSetStringMacro(Variable);

private:
  vtkPVEnvironmentInformation(const vtkPVEnvironmentInformation&) = delete;
  void operator=(const vtkPVEnvironmentInformation&) = delete;
};

#endif
