// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkCPCxxHelper_h
#define vtkCPCxxHelper_h

#include "vtkObject.h"
#include "vtkPVCatalystModule.h" // For windows import/export of shared libraries
#include "vtkSmartPointer.h"     // needed for vtkSmartPointer.
#include "vtkWeakPointer.h"      // needed for vtkWeakPointer.

/// @ingroup CoProcessing
/// Singleton class for initializing without python.
/// The vtkCPCxxHelper instance is created on the first call to
/// vtkCPCxxHelper::New(), subsequent calls return the same instance (but with
/// an increased reference count). When the caller is done with the
/// vtkCPCxxHelper instance, it should simply call Delete() or UnRegister() on
/// it. When the last caller of vtkCPCxxHelper::New() releases the reference,
/// the singleton instance will be cleaned up.
class VTKPVCATALYST_EXPORT vtkCPCxxHelper : public vtkObject
{
public:
  static vtkCPCxxHelper* New();
  vtkTypeMacro(vtkCPCxxHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkCPCxxHelper();
  ~vtkCPCxxHelper() override;

private:
  vtkCPCxxHelper(const vtkCPCxxHelper&) = delete;
  void operator=(const vtkCPCxxHelper&) = delete;

  /// The singleton instance of the class.
  static vtkWeakPointer<vtkCPCxxHelper> Instance;
  static bool ParaViewExternallyInitialized;
};

#endif
