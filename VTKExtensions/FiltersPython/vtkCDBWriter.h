// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkCDBWriter
 * @brief A Cinema Database writer
 *
 * A writer that generates a Cinema database. This uses
 * [`cinemascience`](https://github.com/cinemascience/cinemasci) Python package
 * to generate the database file.
 */

#ifndef vtkCDBWriter_h
#define vtkCDBWriter_h

#include "vtkPVVTKExtensionsFiltersPythonModule.h" //needed for exports
#include "vtkTableAlgorithm.h"

class VTKPVVTKEXTENSIONSFILTERSPYTHON_EXPORT vtkCDBWriter : public vtkTableAlgorithm
{
public:
  static vtkCDBWriter* New();
  vtkTypeMacro(vtkCDBWriter, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the path under which do generate the database.
   * This is expected to be a directory.
   */
  vtkSetStringMacro(Path);
  vtkGetStringMacro(Path);
  ///@}

  /**
   * Causes the writer to write the input data.
   */
  void Write();

protected:
  vtkCDBWriter();
  ~vtkCDBWriter() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkCDBWriter(const vtkCDBWriter&) = delete;
  void operator=(const vtkCDBWriter&) = delete;
  char* Path;
};

#endif
