/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMEnsembleReaderProxy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMEnsembleDataReaderProxy
 *
 * Examines file paths found in ensemble data files (.pve) and creates readers
 * that can read those files. Sets the correct reader for each file on the
 * corresponding VTK object.
*/

#ifndef vtkSMEnsembleDataReaderProxy_h
#define vtkSMEnsembleDataReaderProxy_h

#include "vtkPVServerManagerDefaultModule.h" // for export
#include "vtkSMSourceProxy.h"

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMEnsembleDataReaderProxy : public vtkSMSourceProxy
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
