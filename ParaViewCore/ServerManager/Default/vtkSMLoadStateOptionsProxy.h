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
 * @brief
 *
 */

#ifndef vtkSMLoadStateOptionsProxy_h
#define vtkSMLoadStateOptionsProxy_h

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMProxy.h"

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMLoadStateOptionsProxy : public vtkSMProxy
{
public:
  static vtkSMLoadStateOptionsProxy* New();
  vtkTypeMacro(vtkSMLoadStateOptionsProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Set the state file to load. This may read the file and collect information
   * about the file. Returns false if the filename is invalid or cannot be read.
   */
  virtual bool PrepareToLoad(const char* statefilename);

  /**
   * Check if state file has any data files.
   * @returns ...
   */
  virtual bool HasDataFiles();

  /**
   * Do the state loading.
   */
  virtual bool Load();

protected:
  vtkSMLoadStateOptionsProxy();
  ~vtkSMLoadStateOptionsProxy();

private:
  vtkSMLoadStateOptionsProxy(const vtkSMLoadStateOptionsProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMLoadStateOptionsProxy&) VTK_DELETE_FUNCTION;
};

#endif
