/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVServerOptions - ParaView options storage
// .SECTION Description
// An object of this class represents a storage for ParaView options
//
// These options can be retrieved during run-time, set using configuration file
// or using Command Line Arguments.

#ifndef __vtkPVServerOptions_h
#define __vtkPVServerOptions_h

#include "vtkPVOptions.h"

class vtkPVServerOptionsInternals;

class VTK_EXPORT vtkPVServerOptions : public vtkPVOptions
{
public:
  static vtkPVServerOptions* New();
  vtkTypeMacro(vtkPVServerOptions,vtkPVOptions);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Pass in the name and the attributes for all tags that are not Options.
  // If it returns 1, then it is successful, and 0 if it failed.
  virtual int ParseExtraXMLTag(const char* name, const char** atts);

  // Description:
  // Get information about machines used in a data or render server.
  double GetEyeSeparation();
  unsigned int GetNumberOfMachines();
  const char* GetMachineName(unsigned int idx);
  const char* GetDisplayName(unsigned int idx);
  double* GetLowerLeft(unsigned int idx);
  double* GetLowerRight(unsigned int idx);
  double* GetUpperRight(unsigned int idx);

protected:
  // Description:
  // Add machine information from the xml tag <Machine ....>
  int AddMachineInformation(const char** atts);

  // Description:
  // Add eye separation information from the xml tag <EyeSeparation ...>
  int AddEyeSeparationInformation(const char** atts);

  // Description:
  // Default constructor.
  vtkPVServerOptions();

  // Description:
  // Destructor.
  virtual ~vtkPVServerOptions();

  virtual void Initialize();

private:
  vtkPVServerOptions(const vtkPVServerOptions&); // Not implemented
  void operator=(const vtkPVServerOptions&); // Not implemented

  vtkPVServerOptionsInternals* Internals;
};

#endif // #ifndef __vtkPVServerOptions_h

