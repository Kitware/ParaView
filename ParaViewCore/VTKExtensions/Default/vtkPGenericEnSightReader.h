/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPGenericEnSightReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPGenericEnSightReader - class to read any type of EnSight files
// .SECTION Description
// The class vtkPGenericEnSightReader allows the user to read an EnSight data
// set without a priori knowledge of what type of EnSight data set it is.

#ifndef __vtkPGenericEnSightReader_h
#define __vtkPGenericEnSightReader_h

#include "vtkGenericEnSightReader.h"

class vtkCallbackCommand;
class vtkDataArrayCollection;
class vtkDataArraySelection;
class vtkIdListCollection;

class VTK_EXPORT vtkPGenericEnSightReader : public vtkGenericEnSightReader
{
public:
  static vtkPGenericEnSightReader *New();
  vtkTypeMacro(vtkPGenericEnSightReader, vtkGenericEnSightReader);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPGenericEnSightReader();
  ~vtkPGenericEnSightReader();

  virtual int RequestInformation(vtkInformation*,
                                 vtkInformationVector**,
                                 vtkInformationVector*);

  // Description:
  // Multi Process cache. Will be read a lot of times.
  int GetMultiProcessLocalProcessId();
  // Description:
  // Multi Process cache. Will be read a lot of times.
  int GetMultiProcessNumberOfProcesses();

  int MultiProcessLocalProcessId;
  int MultiProcessNumberOfProcesses;

private:
  vtkPGenericEnSightReader(const vtkPGenericEnSightReader&);  // Not implemented.
  void operator=(const vtkPGenericEnSightReader&);  // Not implemented.
};

#endif
