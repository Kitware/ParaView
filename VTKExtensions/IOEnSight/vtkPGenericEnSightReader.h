// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPGenericEnSightReader
 * @brief   class to read any type of EnSight files
 *
 * The class vtkPGenericEnSightReader allows the user to read an EnSight data
 * set without a priori knowledge of what type of EnSight data set it is.
 */

#ifndef vtkPGenericEnSightReader_h
#define vtkPGenericEnSightReader_h

#include "vtkGenericEnSightReader.h"
#include "vtkPVVTKExtensionsIOEnSightModule.h" //needed for exports

class vtkCallbackCommand;
class vtkDataArrayCollection;
class vtkDataArraySelection;
class vtkIdListCollection;

class VTKPVVTKEXTENSIONSIOENSIGHT_EXPORT vtkPGenericEnSightReader : public vtkGenericEnSightReader
{
public:
  static vtkPGenericEnSightReader* New();
  vtkTypeMacro(vtkPGenericEnSightReader, vtkGenericEnSightReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPGenericEnSightReader();
  ~vtkPGenericEnSightReader() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Multi Process cache. Will be read a lot of times.
   */
  int GetMultiProcessLocalProcessId();
  /**
   * Multi Process cache. Will be read a lot of times.
   */
  int GetMultiProcessNumberOfProcesses();

  int MultiProcessLocalProcessId;
  int MultiProcessNumberOfProcesses;

private:
  vtkPGenericEnSightReader(const vtkPGenericEnSightReader&) = delete;
  void operator=(const vtkPGenericEnSightReader&) = delete;
};

#endif
