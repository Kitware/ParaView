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
// .NAME vtkPVEnvironmentInformation - Information object that can
// be used to obtain values of environment variables.
// .SECTION Description
// vtkPVEnvironmentInformation can be used to get values of environment
// variables.

#ifndef __vtkPVEnvironmentInformation_h
#define __vtkPVEnvironmentInformation_h

#include "vtkPVInformation.h"

class VTK_EXPORT vtkPVEnvironmentInformation : public vtkPVInformation
{
public:
  static vtkPVEnvironmentInformation* New();
  vtkTypeMacro(vtkPVEnvironmentInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  // The object must be a vtkPVEnvironmentInformationHelper.
  virtual void CopyFromObject(vtkObject* object);

  //BTX
  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);

  //ETX

  // Description:
  // Get the value of an environment variable
  vtkGetStringMacro(Variable);

protected:
  vtkPVEnvironmentInformation();
  ~vtkPVEnvironmentInformation();

  char* Variable;     // value of an environment variable

  vtkSetStringMacro(Variable);

private:
  vtkPVEnvironmentInformation(const vtkPVEnvironmentInformation&); // Not implemented.
  void operator=(const vtkPVEnvironmentInformation&); // Not implemented.
};


#endif

