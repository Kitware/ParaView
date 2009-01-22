/*=========================================================================

  Program:   ParaView
  Module:    vtkCleanArrays.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCleanArrays - filter used to remove partial arrays across processes.
// .SECTION Description
// vtkCleanArrays is a filter used to remove partial arrays in a vtkDataSet
// across processes. Empty dataset on any processes is ignored i.e. it does not
// affect the arrays on any processes.

#ifndef __vtkCleanArrays_h
#define __vtkCleanArrays_h

#include "vtkDataSetAlgorithm.h"

class vtkMultiProcessController;

class VTK_EXPORT vtkCleanArrays : public vtkDataSetAlgorithm
{
public:
  static vtkCleanArrays* New();
  vtkTypeRevisionMacro(vtkCleanArrays, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The user can set the controller used for inter-process communication. By
  // default set to the global communicator.
  void SetController(vtkMultiProcessController *controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

//BTX
protected:
  vtkCleanArrays();
  ~vtkCleanArrays();

  virtual int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  vtkMultiProcessController* Controller;
private:
  vtkCleanArrays(const vtkCleanArrays&); // Not implemented
  void operator=(const vtkCleanArrays&); // Not implemented

public:
  class vtkArrayData;
  class vtkArraySet;
//ETX
};

#endif

