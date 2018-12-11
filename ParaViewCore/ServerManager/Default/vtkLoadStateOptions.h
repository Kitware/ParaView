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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(DataDirectory, std::string);

  /**
   * Attempts to locate a file specified by filepath in the directory
   * specified by the DataDirectory member variable. If the file
   * cannot be found, the original filepath is returned. Set the
   * isPath parameter to 0 if you want to locate a file, and set it
   * to 1 if your aim is to replace the directory in the filepath with
   * the DataDirectory member variable if the new directory exists. This
   * can be useful for determining how a file prefix should be changed.
   */
  std::string LocateFileInDirectory(const std::string& filepath, int isPath);

protected:
  vtkLoadStateOptions();
  ~vtkLoadStateOptions() override;

  std::string DataDirectory;

private:
  vtkLoadStateOptions(const vtkLoadStateOptions&) = delete;
  void operator=(const vtkLoadStateOptions&) = delete;
};

#endif
