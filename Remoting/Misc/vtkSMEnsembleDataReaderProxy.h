// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMEnsembleDataReaderProxy
 *
 * Examines file paths found in ensemble data files (.pve) and creates readers
 * that can read those files. Sets the correct reader for each file on the
 * corresponding VTK object.
 */

#ifndef vtkSMEnsembleDataReaderProxy_h
#define vtkSMEnsembleDataReaderProxy_h

#include "vtkRemotingMiscModule.h" // for export
#include "vtkSMSourceProxy.h"

class VTKREMOTINGMISC_EXPORT vtkSMEnsembleDataReaderProxy : public vtkSMSourceProxy
{
public:
  vtkTypeMacro(vtkSMEnsembleDataReaderProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkSMEnsembleDataReaderProxy* New();

  void UpdateVTKObjects() override;

protected:
  vtkSMEnsembleDataReaderProxy();
  ~vtkSMEnsembleDataReaderProxy() override;

  void SetPropertyModifiedFlag(const char* name, int flag) override;

  bool FileNamePotentiallyModified;

private:
  bool FetchFileNames();

  vtkSMEnsembleDataReaderProxy(const vtkSMEnsembleDataReaderProxy&) = delete;
  void operator=(const vtkSMEnsembleDataReaderProxy&) = delete;
};

#endif
