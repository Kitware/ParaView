/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkCPPythonProcessor_h
#define __vtkCPPythonProcessor_h

#include "vtkCPProcessor.h"
#include "vtkPVPythonCatalystModule.h" // For windows import/export of shared libraries

/// @ingroup CoProcessing
/// CoProcessing code should use vtkCPPythonProcessor instead of
/// vtkCPProcessor when if they need Python processing capabilities.
class VTKPVPYTHONCATALYST_EXPORT vtkCPPythonProcessor : public vtkCPProcessor
{
public:
  static vtkCPPythonProcessor* New();
  vtkTypeMacro(vtkCPPythonProcessor, vtkCPProcessor);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkCPPythonProcessor();
  ~vtkCPPythonProcessor();

  /// Create a new instance of the InitializationHelper.
  /// Overridden to create vtkCPPythonHelper as the initializer.
  virtual vtkObject* NewInitializationHelper();

private:
  vtkCPPythonProcessor(const vtkCPPythonProcessor&); // Not implemented
  void operator=(const vtkCPPythonProcessor&); // Not implemented
//ETX
};

#endif
