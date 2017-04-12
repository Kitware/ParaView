/*=========================================================================

  Program:   ParaView
  Module:    vtkLoadStateOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLoadStateOptions
 * @brief   supports locating files in directory when loading a state file.
 *
 * vtkLoadStateOptions supports vtkSMLoadStateOptionsProxy by locating data
 * files in a directory on the data server specified by the user.
 */

#ifndef vtkLoadStateOptions_h
#define vtkLoadStateOptions_h

#include "vtkObject.h"
#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include <string>                            // needed for std::string

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkLoadStateOptions : public vtkObject
{
public:
  static vtkLoadStateOptions* New();
  vtkTypeMacro(vtkLoadStateOptions, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkSetMacro(DataDirectory, std::string);

  std::string LocateFileInDirectory(const std::string& filepath);

protected:
  vtkLoadStateOptions();
  ~vtkLoadStateOptions();

  std::string DataDirectory;

private:
  vtkLoadStateOptions(const vtkLoadStateOptions&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLoadStateOptions&) VTK_DELETE_FUNCTION;
};

#endif
