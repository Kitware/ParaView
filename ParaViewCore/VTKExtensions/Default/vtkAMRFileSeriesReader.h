/*=========================================================================

  Program:   ParaView
  Module:    vtkFileSeriesReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkAMRFileSeriesReader_h
#define vtkAMRFileSeriesReader_h

#include "vtkFileSeriesReader.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkAMRFileSeriesReader : public vtkFileSeriesReader
{
public:
  static vtkAMRFileSeriesReader* New();
  vtkTypeMacro(vtkAMRFileSeriesReader, vtkFileSeriesReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  virtual int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

  virtual int RequestUpdateTime(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  virtual int RequestUpdateTimeDependentInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

private:
  vtkAMRFileSeriesReader();
  vtkAMRFileSeriesReader(const vtkAMRFileSeriesReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAMRFileSeriesReader&) VTK_DELETE_FUNCTION;
};

#endif
