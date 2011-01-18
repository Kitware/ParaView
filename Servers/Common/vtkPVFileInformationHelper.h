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
// .NAME vtkPVFileInformationHelper - server side object used to gather information
// from, by vtkPVFileInformation.
// .SECTION Description
// When collection information, ProcessModule cannot pass parameters to
// the information object. In case of vtkPVFileInformation, we need data on
// the server side such as which directory/file are we concerned with. 
// To make such information available, we use vtkPVFileInformationHelper.
// One creates a server side representation of vtkPVFileInformationHelper and
// sets attributes on it, then requests a gather information on the helper object.

#ifndef __vtkPVFileInformationHelper_h
#define __vtkPVFileInformationHelper_h

#include "vtkObject.h"

class VTK_EXPORT vtkPVFileInformationHelper : public vtkObject
{
public:
  static vtkPVFileInformationHelper* New();
  vtkTypeMacro(vtkPVFileInformationHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the path to the directory/file whose information we are 
  // interested in. This is ignored when SpecialDirectories is set
  // to True.
  vtkSetStringMacro(Path);
  vtkGetStringMacro(Path);

  // Description:
  // Get/Set the current working directory. This is needed if Path is 
  // relative. The relative path will be converted to absolute path using the
  // working directory specified before obtaining information about it.
  // If 0 (default), then the application's current working directory will be 
  // used to flatten relative paths.
  vtkSetStringMacro(WorkingDirectory);
  vtkGetStringMacro(WorkingDirectory);

  // Description:
  // Get/Set if the we should attempt to get the information
  // of contents if Path is a directory.
  // Default value is 0. 
  // This is ignored when SpecialDirectories is set to True.
  vtkGetMacro(DirectoryListing, int);
  vtkSetMacro(DirectoryListing, int);
  vtkBooleanMacro(DirectoryListing, int);

  // Description:
  // Get/Set if the query is for special directories.
  // Off by default. If set to true, Path and DirectoryListing 
  // are ignored and the vtkPVFileInformation object
  // is populated with information about sepcial directories
  // such as "My Documents", "Destop" etc on Windows systems
  // and "Home" on Unix based systems.
  vtkGetMacro(SpecialDirectories, int);
  vtkSetMacro(SpecialDirectories, int);
  vtkBooleanMacro(SpecialDirectories, int);

  // Description:
  // When on, while listing a directory, 
  // whenever a group of files is encountered, we verify
  // the type/accessibility of only the first file in the group
  // and assume that all other have similar permissions.
  // On by default.
  vtkGetMacro(FastFileTypeDetection, int);
  vtkSetMacro(FastFileTypeDetection, int);

  // Description:
  // Returns the platform specific path separator.
  vtkGetStringMacro(PathSeparator);

  // Description:
  // Returns if this->Path is a readable file.
  bool GetActiveFileIsReadable();

  // Description:
  // Returns if this->Path is a directory.
  bool GetActiveFileIsDirectory();

protected:
  vtkPVFileInformationHelper();
  ~vtkPVFileInformationHelper();

  char* Path;
  char* WorkingDirectory;
  int DirectoryListing;
  int SpecialDirectories;
  int FastFileTypeDetection;

  char* PathSeparator;
  vtkSetStringMacro(PathSeparator);
private:
  vtkPVFileInformationHelper(const vtkPVFileInformationHelper&); // Not implemented.
  void operator=(const vtkPVFileInformationHelper&); // Not implemented.
};

#endif

