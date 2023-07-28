// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkAMRFileSeriesReader_h
#define vtkAMRFileSeriesReader_h

#include "vtkFileSeriesReader.h"
#include "vtkPVVTKExtensionsIOAMRModule.h" //needed for exports

class VTKPVVTKEXTENSIONSIOAMR_EXPORT vtkAMRFileSeriesReader : public vtkFileSeriesReader
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
