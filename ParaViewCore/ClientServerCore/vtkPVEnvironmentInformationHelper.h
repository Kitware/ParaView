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
// .NAME vtkPVEnvironmentInformationHelper - Helper object that can
// be used to obtain information about an environment.
// .SECTION Description
// vtkPVEnvironmentInformationHelper can be used to get values of environment
// variables.

#ifndef __vtkPVEnvironmentInformationHelper_h
#define __vtkPVEnvironmentInformationHelper_h

#include "vtkObject.h"

class VTK_EXPORT vtkPVEnvironmentInformationHelper : public vtkObject
{
public:
  static vtkPVEnvironmentInformationHelper* New();
  vtkTypeMacro(vtkPVEnvironmentInformationHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get/Set the environment variable that we want to get the value of
  vtkSetStringMacro(Variable);
  vtkGetStringMacro(Variable);

protected:
  vtkPVEnvironmentInformationHelper();
  ~vtkPVEnvironmentInformationHelper();

  char* Variable;

private:
  vtkPVEnvironmentInformationHelper(const vtkPVEnvironmentInformationHelper&); // Not implemented.
  void operator=(const vtkPVEnvironmentInformationHelper&); // Not implemented.
};


#endif

