/*=========================================================================

  Program:   ParaView
  Module:    vtkMergeArrays.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMergeArrays - Multiple inputs with same geometry, one output.
// .SECTION Description
// vtkMergeArrays Expects that all inputs have the same geometry.
// Arrays from all inputs are put into out output.
// The filter checks for a consistent number of points and cells, but
// not check any more.  Any inputs which do not have the correct number
// of points and cells are ignored.

#ifndef __vtkMergeArrays_h
#define __vtkMergeArrays_h

#include "vtkDataSetAlgorithm.h"

class vtkDataSet;

class VTK_EXPORT vtkMergeArrays : public vtkDataSetAlgorithm
{
public:
  static vtkMergeArrays *New();

  vtkTypeMacro(vtkMergeArrays,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkMergeArrays();
  ~vtkMergeArrays();

  virtual int RequestData(vtkInformation*, 
                          vtkInformationVector**, 
                          vtkInformationVector*);
  
  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkMergeArrays(const vtkMergeArrays&);  // Not implemented.
  void operator=(const vtkMergeArrays&);  // Not implemented.
};


#endif


