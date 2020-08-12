/*=========================================================================

  Program:   ParaView
  Module:    vtkSMFileUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMFileUtilities
 * @brief file system utilties
 *
 * vtkSMFileUtilities can be used to make various file-system related queries
 * and updates by applications. It is intended to be used instead of the
 * `("misc", "Directory")` proxy (or `vtkSMDirectoryProxy`) since this class can
 * better support MPI and symmetric-MPI mode of operation. It does that as
 * follows:
 *
 * * Any API call that modifies the directory structure only gets called on the
 *   root-node, and never on the satellites. This avoids unnecessary overheads
 *   encountered by the filesystem to avoid race-conditions on parallel file
 *   systems.
 *
 * * In symmetric-MPI mode, since the server-manager level code is called on all
 *   ranks including the satellites, this class ensures that while the
 *   filesystem updates are only triggered on the root node, the status is
 *   correctly shared among all the satellites. Thus the function call will
 *   return correct status on all ranks.
 */

#ifndef vtkSMFileUtilities_h
#define vtkSMFileUtilities_h

#include "vtkRemotingServerManagerModule.h" // for exports
#include "vtkSMSessionObject.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMFileUtilities : public vtkSMSessionObject
{
public:
  static vtkSMFileUtilities* New();
  vtkTypeMacro(vtkSMFileUtilities, vtkSMSessionObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Make a directory on the specified location. Returns true on success or if
   * the directory already exists.
   *
   * In symmetric MPI mode, this method must be called on all
   * MPI ranks otherwise it will cause a deadlock.
   */
  bool MakeDirectory(const std::string& name, vtkTypeUInt32 location);

  /**
   * Delete a directory on the specified location. Returns true on success.
   *
   * In symmetric MPI mode, this method must be called on all
   * MPI ranks otherwise it will cause a deadlock.
   */
  bool DeleteDirectory(const std::string& name, vtkTypeUInt32 location);

  /**
   * Rename a directory on the specified location. Returns true on success.
   *
   * In symmetric MPI mode, this method must be called on all
   * MPI ranks otherwise it will cause a deadlock.
   */
  bool RenameDirectory(const std::string& name, const std::string& newname, vtkTypeUInt32 location);

protected:
  vtkSMFileUtilities();
  ~vtkSMFileUtilities();

private:
  vtkSMFileUtilities(const vtkSMFileUtilities&) = delete;
  void operator=(const vtkSMFileUtilities&) = delete;
};

#endif
