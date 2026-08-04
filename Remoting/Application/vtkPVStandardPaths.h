// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVStandardPaths
 * @brief   Utilities methods to get standard paths.
 *
 * Methods to get useful paths depending on the current
 * operating system and on the running application:
 * - system directories depends only on the system
 * - install directories are relative to the running application
 * - user directory is a conventional user space. It is expected to be writeable.
 *
 * Different environment variables are taken into account, trying to
 * respect diffent system convention:
 * - unix-like: https://specifications.freedesktop.org/basedir-spec/0.8
 */

#ifndef vtkPVStandardPaths_h
#define vtkPVStandardPaths_h

#include "vtkRemotingApplicationModule.h"

#include <string>
#include <vector>

/**
 * @class   vtkPVStandardPaths
 * @brief   Utilities methods to get standard paths.
 *
 * This class has only static methods to access standard paths used by ParaView
 * and is not meant to be instantiated.
 */
class VTKREMOTINGAPPLICATION_EXPORT vtkPVStandardPaths
{
public:
  /**
   * Return system data directories, depending on operating system:
   * - Windows: Uses COMMON_APPDATA:
   * - MacOS - linux: Honorates XDG_DATA_DIRS, falling back to /usr/share and /usr/local/share.
   * https://specifications.freedesktop.org/basedir-spec/0.8/
   */
  static std::vector<std::string> GetSystemDirectories();

  /**
   * Returns paths relative to install directory.
   * - the exectutable dir
   * - the `<install>` dir: the executable dir or its parent if executable is under a "bin" dir.
   * - `<install>/lib` `<install>/share/paraview-<version>`
   * - MacOs specific:
   *   -  package `<root>`: `<install>/../../..`
   *   - `lib`, `lib-paraview-<version>`, `Support` as `<root>` subdirs.
   */
  static std::vector<std::string> GetInstallDirectories();

  /**
   * Get directory for user settings file. The last character is always the
   * file path separator appropriate for the system.
   * - Windows: honors `APPDATA` environment variable
   * - linux/MacOs: honors xdg convention:
   *   - `$XDG_DATA_HOME` environment first
   *   - `$HOME/.config/<organization-name>` then
   *   see: https://specifications.freedesktop.org/basedir-spec/0.8/
   */
  static std::string GetUserSettingsDirectory();

  /**
   * Get file path for the user settings file.
   * Construct server settings file path:
   *  `<GetUserSettingsDirectory()>/<GetApplicationName()>-UserSettings.json`
   * @see vtkInitializationHelper::GetApplicationName()
   */
  static std::string GetUserSettingsFilePath();

private:
  // Delete all constructors and operators to avoid instantiation of this class.
  vtkPVStandardPaths() = delete;
  ~vtkPVStandardPaths() = delete;
  vtkPVStandardPaths(const vtkPVStandardPaths&) = delete;
  void operator=(const vtkPVStandardPaths&) = delete;
};

#endif
