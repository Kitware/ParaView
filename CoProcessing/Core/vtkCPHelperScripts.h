/*=========================================================================

  Program:   ParaView
  Module:    vtkCPHelperScripts.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkCPHelperScripts_h
#define vtkCPHelperScripts_h

#include "vtkObject.h"
#include "vtkCoProcessorModule.h" // For windows import/export of shared libraries

/// @ingroup CoProcessing
/// Static API used to access Python Helper scripts.
class VTKCOPROCESSOR_EXPORT vtkCPHelperScripts : public vtkObject
{
public:
  static vtkCPHelperScripts* New();
  vtkTypeMacro(vtkCPHelperScripts,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  static const char* GetPythonHelperScript();

protected:
  vtkCPHelperScripts();
  virtual ~vtkCPHelperScripts();

private:
  vtkCPHelperScripts(const vtkCPHelperScripts&); // Not implemented
  void operator=(const vtkCPHelperScripts&); // Not implemented
};

#endif
