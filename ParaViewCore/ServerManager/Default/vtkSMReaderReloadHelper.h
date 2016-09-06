/*=========================================================================

  Program:   ParaView
  Module:    vtkSMReaderReloadHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMReaderReloadHelper - helper to help reload a reader.
// .SECTION Description
// vtkSMReaderReloadHelper helps make a reader reload its files. There are two
// ways of reloading: reload existing data files, or extend the file series
// with any new files that are now available. This class supports both. Use
// vtkSMReaderReloadHelper::ReloadFiles() to reload existing files (potentially
// using a specific property on the reader proxy for reloading, if available).
// Use vtkSMReaderReloadHelper::ExtendFileSeries() to detect new files in a file
// series and update the reader to use those.
#ifndef vtkSMReaderReloadHelper_h
#define vtkSMReaderReloadHelper_h

#include "vtkObject.h"
#include "vtkPVServerManagerDefaultModule.h" //needed for exports

class vtkSMSourceProxy;

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMReaderReloadHelper : public vtkObject
{
public:
  static vtkSMReaderReloadHelper* New();
  vtkTypeMacro(vtkSMReaderReloadHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if its possible to reload data files for the given proxy i.e.
  // checks if the proxy is a reader. If not, returns false.
  virtual bool SupportsReload(vtkSMSourceProxy* proxy);

  // Description:
  // Returns true if the reader supports file series.
  virtual bool SupportsFileSeries(vtkSMSourceProxy* proxy);

  // Description:
  // Make the reader reload its data file(s). If a reader supports API to
  // support such reloads in  a smart way, then the reader proxy should use the
  // `<ReloadFiles />` hint to name the property. In that case, this method will
  // use that property. Otherwise, it will use vtkSMProxy::RecreateVTKObjects()
  // to simply recreate the VTK object and update its state.
  virtual bool ReloadFiles(vtkSMSourceProxy* proxy);

  // Description:
  // Attempts to find more files in a file series specified on the reader proxy
  // and all those files to the reader. This should be called only when
  // IsFileSeriesCapable() returns true.
  virtual bool ExtendFileSeries(vtkSMSourceProxy* proxy);

protected:
  vtkSMReaderReloadHelper();
  ~vtkSMReaderReloadHelper();

private:
  vtkSMReaderReloadHelper(const vtkSMReaderReloadHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMReaderReloadHelper&) VTK_DELETE_FUNCTION;
};

#endif
