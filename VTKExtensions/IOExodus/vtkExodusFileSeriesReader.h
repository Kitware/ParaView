// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkExodusFileSeriesReader
 * @brief   meta-reader to read Exodus file series from simulation restarts
 *
 *
 *
 * Add some special sauce to the superclass that allows it to work with the
 * parallel Exodus reader.  Specifically, changing the file name causes
 * the selected output arrays to be cleared out.  This class saves and
 * restores the information.
 *
 */

#ifndef vtkExodusFileSeriesReader_h
#define vtkExodusFileSeriesReader_h

#include "vtkFileSeriesReader.h"
#include "vtkPVVTKExtensionsIOExodusModule.h" //needed for exports

class VTKPVVTKEXTENSIONSIOEXODUS_EXPORT vtkExodusFileSeriesReader : public vtkFileSeriesReader
{
public:
  vtkTypeMacro(vtkExodusFileSeriesReader, vtkFileSeriesReader);
  static vtkExodusFileSeriesReader* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkExodusFileSeriesReader();
  ~vtkExodusFileSeriesReader() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestInformationForInput(
    int index, vtkInformation* request, vtkInformationVector* outputVector) override;

  // Replaces the filenames, which probably represents partitions of the data,
  // with a set of files where each represents a set of solution files for one
  // of the simulation restarts.
  virtual void FindRestartedResults();

private:
  vtkExodusFileSeriesReader(const vtkExodusFileSeriesReader&) = delete;
  void operator=(const vtkExodusFileSeriesReader&) = delete;
};

#endif // vtkExodusFileSeriesReader_h
