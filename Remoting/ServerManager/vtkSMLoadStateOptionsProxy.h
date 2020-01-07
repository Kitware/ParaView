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
 * @brief proxy for state loading options
 *
 * vtkSMLoadStateOptionsProxy is the proxy used to setup options when loading a
 * state file. These including choosing how to handle data files referenced in
 * the state. This proxy supports multiple ways to let the user update the files
 * being loaded in the state file.
 *
 * To use this, create the `("options", "LoadStateOptions")` proxy using the
 * session's proxy manager and call `PrepareToLoad` with the state file to load.
 * This will parse the state file and setup internal data structures. At this
 * point, you can call `HasDataFiles` method to determine if the state file has
 * any data files. This can be used to determine if the application should
 * prompt the user with options, for example. One can change options using
 * properties on the options proxy itself. Following `PrepareToLoad`, the proxy
 * will present additional properties that allows the user to pick filenames for
 * readers in the state file. Call `Load` to load the state file.
 *
 * This proxy is not intended for reuse. Simply create a new one for each state
 * to load.
 */
#ifndef vtkSMLoadStateOptionsProxy_h
#define vtkSMLoadStateOptionsProxy_h

#include "vtkNew.h"                         // for vtkNew
#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMProxy.h"
#include <vector> // needed for std::vector

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMLoadStateOptionsProxy : public vtkSMProxy
{
public:
  static vtkSMLoadStateOptionsProxy* New();
  vtkTypeMacro(vtkSMLoadStateOptionsProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
   * Do the state loading using the values for properties on this proxy to
   * configure the state loading i.e. update data files, etc.
   */
  virtual bool Load();

  enum
  {
    USE_FILES_FROM_STATE = 0,
    USE_DATA_DIRECTORY = 1,
    CHOOSE_FILES_EXPLICITLY = 2
  };

protected:
  vtkSMLoadStateOptionsProxy();
  ~vtkSMLoadStateOptionsProxy() override;

  /**
   * Locates the files in the user specified directory and updates the paths
   * in the state file XML. Set the isPath parameter to 1 for file
   * paths that are prefixes, 0 otherwise. If the clearFilenameIfNotFound
   * option is set to true it will set the filenames that are not in the
   * directory to empty string rather than leave them as the filenames from
   * the state file XML.
   *
   * @returns false if any of the files were not found
   */
  bool LocateFilesInDirectory(
    std::vector<std::string>& filepaths, int path, bool clearFilenameIfNotFound);

  vtkSetStringMacro(StateFileName);
  char* StateFileName;

  /**
   * The three options for how to specify the data file locations.
   */
  int DataFileOptions;

  /**
   * When the number of parent directories for a file series reaches this number assume it will
   * match for the rest of the file series.
   */
  int PathMatchingThreshold;

  /**
   * When this is set to true, USE_DATA_DIRECTORY mode will fail when a file is not in the
   * data directory rather than defaulting back to the absolute path in the state file.
   */
  bool OnlyUseFilesInDataDirectory;

private:
  class vtkInternals;
  vtkInternals* Internals;

  vtkSMLoadStateOptionsProxy(const vtkSMLoadStateOptionsProxy&) = delete;
  void operator=(const vtkSMLoadStateOptionsProxy&) = delete;
};

#endif
