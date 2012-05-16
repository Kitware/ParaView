/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVMultiServerDataSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVMultiServerDataSource
// .SECTION Description
// VTK class that handle the fetch of remote data

#ifndef __vtkMultiServerDataSource_h
#define __vtkMultiServerDataSource_h

#include "vtkDataObjectAlgorithm.h"

class vtkSMSourceProxy;
class vtkInformation;
class vtkInformationVector;

class VTK_EXPORT vtkPVMultiServerDataSource : public vtkDataObjectAlgorithm
{
public:
  static vtkPVMultiServerDataSource *New();
  vtkTypeMacro(vtkPVMultiServerDataSource,vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Bind vtk object with a given external proxy
  virtual void SetExternalProxy(vtkSMSourceProxy* proxyFromAnotherServer, int portNumber = 0);

  // Description:
  // Method that need to be called when the data has changed and need to be updated...
  virtual void FetchData(vtkDataObject* dataObjectToFill);

//BTX
protected:
  vtkPVMultiServerDataSource();
  ~vtkPVMultiServerDataSource();

  // call 1
  virtual int RequestDataObject(vtkInformation *,
                                vtkInformationVector** vtkNotUsed(inputVector),
                                vtkInformationVector* outputVector);

  // call 2
  virtual int RequestInformation(vtkInformation *,
                                 vtkInformationVector **,
                                 vtkInformationVector *outputVector);

  // call 3
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  // call 4
  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *outputVector);

private:
  vtkPVMultiServerDataSource(const vtkPVMultiServerDataSource&);  // Not implemented.
  void operator=(const vtkPVMultiServerDataSource&);  // Not implemented.

  struct vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif
