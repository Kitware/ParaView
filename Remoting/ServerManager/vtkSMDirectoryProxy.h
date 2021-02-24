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
/**
 * @class   vtkSMDirectoryProxy
 *
 * Is a utility proxy to create/delete/rename directories.
 *
 * @sa vtkSMFileUtilities. Using `vtkSMFileUtilities` is recommended over
 * directly using this class since vtkSMFileUtilities handles MPI and parallel
 * file systems better.
*/

#ifndef vtkSMDirectoryProxy_h
#define vtkSMDirectoryProxy_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMProxy.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMDirectoryProxy : public vtkSMProxy
{
public:
  static vtkSMDirectoryProxy* New();
  vtkTypeMacro(vtkSMDirectoryProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Create directory.
   */
  bool MakeDirectory(const char* dir);

  /**
   * Remove a directory.
   */
  bool DeleteDirectory(const char* dir);

  /**
   * Rename a file or directory.
   */
  bool Rename(const char* oldname, const char* newname);

  /**
   * List server side directory
   * NEVER USED IN PARAVIEW, TODO ?
   */
  bool List(const char* dir);

protected:
  vtkSMDirectoryProxy();
  ~vtkSMDirectoryProxy() override;

  bool CallDirectoryMethod(
    const char* method, const char* path, const char* secondaryPath = nullptr);

private:
  vtkSMDirectoryProxy(const vtkSMDirectoryProxy&) = delete;
  void operator=(const vtkSMDirectoryProxy&) = delete;
};

#endif
