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
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestUpdateTime(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestUpdateTimeDependentInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkAMRFileSeriesReader();
  vtkAMRFileSeriesReader(const vtkAMRFileSeriesReader&) = delete;
  void operator=(const vtkAMRFileSeriesReader&) = delete;
};

#endif
