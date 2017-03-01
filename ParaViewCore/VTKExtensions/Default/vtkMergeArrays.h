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
/**
 * @class   vtkMergeArrays
 * @brief   Multiple inputs with same geometry, one output.
 *
 * vtkMergeArrays Expects that all inputs have the same geometry.
 * Arrays from all inputs are put into out output.
 * The filter checks for a consistent number of points and cells, but
 * not check any more.  Any inputs which do not have the correct number
 * of points and cells are ignored.
*/

#ifndef vtkMergeArrays_h
#define vtkMergeArrays_h

#include "vtkDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class vtkDataSet;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkMergeArrays : public vtkDataSetAlgorithm
{
public:
  static vtkMergeArrays* New();

  vtkTypeMacro(vtkMergeArrays, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkMergeArrays();
  ~vtkMergeArrays();

  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

private:
  vtkMergeArrays(const vtkMergeArrays&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMergeArrays&) VTK_DELETE_FUNCTION;
};

#endif
