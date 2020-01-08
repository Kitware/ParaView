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
/**
 * @class   vtkPVMultiServerDataSource
 *
 * VTK class that handle the fetch of remote data
*/

#ifndef vtkPVMultiServerDataSource_h
#define vtkPVMultiServerDataSource_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkRemotingMiscModule.h" //needed for exports

class vtkSMSourceProxy;
class vtkInformation;
class vtkInformationVector;

class VTKREMOTINGMISC_EXPORT vtkPVMultiServerDataSource : public vtkDataObjectAlgorithm
{
public:
  static vtkPVMultiServerDataSource* New();
  vtkTypeMacro(vtkPVMultiServerDataSource, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Bind vtk object with a given external proxy
   */
  virtual void SetExternalProxy(vtkSMSourceProxy* proxyFromAnotherServer, int portNumber = 0);

  /**
   * Method that need to be called when the data has changed and need to be updated...
   */
  virtual void FetchData(vtkDataObject* dataObjectToFill);

protected:
  vtkPVMultiServerDataSource();
  ~vtkPVMultiServerDataSource() override;

  // call 1
  int RequestDataObject(vtkInformation*, vtkInformationVector** vtkNotUsed(inputVector),
    vtkInformationVector* outputVector) override;

  // call 2
  int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override;

  // call 3
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // call 4
  int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override;

private:
  vtkPVMultiServerDataSource(const vtkPVMultiServerDataSource&) = delete;
  void operator=(const vtkPVMultiServerDataSource&) = delete;

  struct vtkInternal;
  vtkInternal* Internal;
};

#endif
