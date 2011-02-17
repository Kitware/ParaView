/*=========================================================================

  Program:   ParaView
  Module:    vtkTestingProcessModuleGUIHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkTestingProcessModuleGUIHelper_h
#define __vtkTestingProcessModuleGUIHelper_h

#include "vtkObject.h"

class VTK_EXPORT vtkTestingProcessModuleGUIHelper : public vtkObject
{
public:
  static vtkTestingProcessModuleGUIHelper* New();
  vtkTypeMacro(vtkTestingProcessModuleGUIHelper,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // run main gui loop from process module
  int Run();

protected:
  vtkTestingProcessModuleGUIHelper();
  virtual ~vtkTestingProcessModuleGUIHelper();

private:
  vtkTestingProcessModuleGUIHelper(const vtkTestingProcessModuleGUIHelper&); // Not implemented
  void operator=(const vtkTestingProcessModuleGUIHelper&); // Not implemented
};

#endif
