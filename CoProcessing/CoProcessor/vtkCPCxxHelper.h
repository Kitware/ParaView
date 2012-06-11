/*=========================================================================

  Program:   ParaView
  Module:    vtkCPCxxHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkCPCxxHelper_h
#define vtkCPCxxHelper_h

#include "vtkCPPipeline.h"
#include "CPWin32Header.h" // For windows import/export of shared libraries

class vtkPVOptions;

/// @ingroup CoProcessing
/// Singleton class for initializing without python.
class COPROCESSING_EXPORT vtkCPCxxHelper : public vtkObject
{
public:
  static vtkCPCxxHelper* New();
  vtkTypeMacro(vtkCPCxxHelper,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkCPCxxHelper();
  virtual ~vtkCPCxxHelper();

private:
  vtkCPCxxHelper(const vtkCPCxxHelper&); // Not implemented
  void operator=(const vtkCPCxxHelper&); // Not implemented

  vtkPVOptions* Options;

  /// The singleton instance of the class.
  static vtkCPCxxHelper* Instance;
};

#endif
