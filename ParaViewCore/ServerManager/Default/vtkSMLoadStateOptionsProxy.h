/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLoadStateOptionsProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMLoadStateOptionsProxy
 * @brief proxy for managing data files when loading a state file.
 *
 *
 * vtkSMLoadStateOptionsProxy provides a dialog to allow a user to change the
 * locations of data files when loading a state file. The user can give a directory
 * where the data files reside or explicitly change the path for each data file.
 */

#ifndef vtkSMLoadStateOptionsProxy_h
#define vtkSMLoadStateOptionsProxy_h

#include "vtkNew.h"
#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"

class vtkFileSequenceParser;

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMLoadStateOptionsProxy : public vtkSMProxy
{
public:
  static vtkSMLoadStateOptionsProxy* New();
  vtkTypeMacro(vtkSMLoadStateOptionsProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Set the state file to load. This may read the file and collect information
   * about the file. Returns false if the filename is invalid or cannot be read.
   */
  virtual bool PrepareToLoad(const char* statefilename);

  /**
   * Check if state file has any data files.
   * @returns whether the state file refers to any data files to be loaded.
   */
  virtual bool HasDataFiles();

  /**
   * Do the state loading.
   */
  virtual bool Load();

protected:
  vtkSMLoadStateOptionsProxy();
  ~vtkSMLoadStateOptionsProxy();

  vtkSetStringMacro(StateFileName);
  char* StateFileName;

private:
  class vtkInternals;
  vtkInternals* Internals;
  vtkNew<vtkFileSequenceParser> SequenceParser;

  vtkSMLoadStateOptionsProxy(const vtkSMLoadStateOptionsProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMLoadStateOptionsProxy&) VTK_DELETE_FUNCTION;
};

#endif
