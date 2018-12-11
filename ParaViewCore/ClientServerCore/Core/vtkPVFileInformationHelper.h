/*=========================================================================

  Program:   ParaView
  Module:    vtkPVFileInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVFileInformationHelper
 * @brief   server side object used to gather information
 * from, by vtkPVFileInformation.
 *
 * When collection information, ProcessModule cannot pass parameters to
 * the information object. In case of vtkPVFileInformation, we need data on
 * the server side such as which directory/file are we concerned with.
 * To make such information available, we use vtkPVFileInformationHelper.
 * One creates a server side representation of vtkPVFileInformationHelper and
 * sets attributes on it, then requests a gather information on the helper object.
*/

#ifndef vtkPVFileInformationHelper_h
#define vtkPVFileInformationHelper_h

#include "vtkObject.h"
#include "vtkPVClientServerCoreCoreModule.h" //needed for exports

#include <string> // needed for std::string

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVFileInformationHelper : public vtkObject
{
public:
  static vtkPVFileInformationHelper* New();
  vtkTypeMacro(vtkPVFileInformationHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the path to the directory/file whose information we are
   * interested in. This is ignored when SpecialDirectories is set
   * to True.
   */
  vtkSetStringMacro(Path);
  vtkGetStringMacro(Path);
  //@}

  //@{
  /**
   * Get/Set the current working directory. This is needed if Path is
   * relative. The relative path will be converted to absolute path using the
   * working directory specified before obtaining information about it.
   * If 0 (default), then the application's current working directory will be
   * used to flatten relative paths.
   */
  vtkSetStringMacro(WorkingDirectory);
  vtkGetStringMacro(WorkingDirectory);
  //@}

  //@{
  /**
   * Get/Set if the we should attempt to get the information
   * of contents if Path is a directory.
   * Default value is 0.
   * This is ignored when SpecialDirectories is set to True.
   */
  vtkGetMacro(DirectoryListing, int);
  vtkSetMacro(DirectoryListing, int);
  vtkBooleanMacro(DirectoryListing, int);
  //@}

  //@{
  /**
   * Get/Set if the query is for special directories.
   * Off by default. If set to true, Path and DirectoryListing
   * are ignored and the vtkPVFileInformation object
   * is populated with information about special directories
   * such as "My Documents", "Desktop" etc on Windows systems
   * and "Home" on Unix based systems.
   */
  vtkGetMacro(SpecialDirectories, int);
  vtkSetMacro(SpecialDirectories, int);
  vtkBooleanMacro(SpecialDirectories, int);
  //@}

  //@{
  /**
   * When on, while listing a directory,
   * whenever a group of files is encountered, we verify
   * the type/accessibility of only the first file in the group
   * and assume that all other have similar permissions.
   * On by default.
   */
  vtkGetMacro(FastFileTypeDetection, int);
  vtkSetMacro(FastFileTypeDetection, int);
  //@}

  //@{
  /**
   * Returns the platform specific path separator.
   */
  vtkGetStringMacro(PathSeparator);
  //@}

  /**
   * Returns if this->Path is a readable file.
   */
  bool GetActiveFileIsReadable();

  /**
   * Returns if this->Path is a directory.
   */
  bool GetActiveFileIsDirectory();

  /**
   * Transform local code page string to UTF8 string
   * on windows only, pass through otherwise
   */
  static std::string LocalToUtf8Win32(const std::string& path);

  /**
   * Transform utf8 string to local code page string
   * on windows only, pass through otherwise
   */
  static std::string Utf8ToLocalWin32(const std::string& path);

  //@{
  /**
   * When off, while listing a directory we skip the expensive fstat call on every file
   * and instead return only their names and basic information about them. Defaults to off.
   * To enable the detailed information like file size and modified time turn this on.
   */
  vtkGetMacro(ReadDetailedFileInformation, bool);
  vtkSetMacro(ReadDetailedFileInformation, bool);
  //@}

protected:
  vtkPVFileInformationHelper();
  ~vtkPVFileInformationHelper() override;

  char* Path;
  char* WorkingDirectory;
  int DirectoryListing;
  int SpecialDirectories;
  int FastFileTypeDetection;

  bool ReadDetailedFileInformation;
  char* PathSeparator;
  vtkSetStringMacro(PathSeparator);

private:
  vtkPVFileInformationHelper(const vtkPVFileInformationHelper&) = delete;
  void operator=(const vtkPVFileInformationHelper&) = delete;
};

#endif
