/*=========================================================================

  Module:    vtkKWDirectoryUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWDirectoryUtilities - Platform independent directory handling.
// .SECTION Description
// vtkKWDirectoryUtilities provides a set of tools for platform
// independent handling of directories, environment variables, and
// program locations.

#ifndef __vtkKWDirectoryUtilities_h
#define __vtkKWDirectoryUtilities_h

#include "vtkObject.h"

class VTK_EXPORT vtkKWDirectoryUtilities : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkKWDirectoryUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);  
  static vtkKWDirectoryUtilities* New();
  
  // Description:
  // Get an environment variable with the given name.  Returns 0 if
  // the variable does not exist.
  const char* GetEnv(const char* key);
  
  // Description:
  // Get the current working directory.
  const char* GetCWD();
  
  // Description:
  // Convert a path to UNIX-style slashes.  Backslashes are replaced
  // with forward slashes.  Trailing slashes are removed, and leading
  // tildas ("~") are replaced with the home directory if the HOME
  // environment variable is set.
  const char* ConvertToUnixSlashes(const char* path);
  
  // Description:
  // Tests the existence of a file.  Returns 1 for exists, 0 otherwise.
  static int FileExists(const char* filename);
  
  // Description:
  // Tests whether a file is a directory.  Returns 1 for yes, 0 for no.
  static int FileIsDirectory(const char* name);
  
  //BTX
  // Description:
  // Get the system path from the PATH environment variable.  Returns
  // an array of pointers to each path entry.  The list is terminated
  // by a 0 pointer.
  const char*const* GetSystemPath();
  //ETX
  
  // Description:
  // Find a program with the given name.  Returns the full path to the
  // executable file, including its name.  If the program is not
  // found, 0 is returned.
  const char* FindProgram(const char* name);
  
  // Description:
  // Get the given directory in a simplified full path format.
  const char* CollapseDirectory(const char* dir);
  
  // Description:
  // Find the location of the executable from the value of argv[0].
  const char* FindSelfPath(const char* argv0);
  
  // Description:
  // Extract the path of a given filename (i.e. its directory path) and 
  // write it to 'path'.
  // Return a pointer to the path (i.e. 'path').
  static const char* GetFilenamePath(const char *filename, char *path);

  // Description:
  // Extract the name of a given filename (i.e. the filename without its 
  // directory path) and write it to 'name'.
  // Return a pointer to the name (i.e. 'name').
  static const char* GetFilenameName(const char *filename, char *name);

  // Description:
  // Extract the extension of a given filename (excluding the dot '.')
  // and write it to 'ext'.
  // Return a pointer to the extension (i.e. 'ext').
  static const char* GetFilenameExtension(const char *filename, char *ext);

  // Description:
  // Remove the extension of a given filename (including the dot '.') 
  // and write it to 'name'.
  // Return a pointer to the name (i.e. 'name').
  static const char* GetFilenameWithoutExtension(
    const char *filename, char *name);

  // Description:
  // Try to locate the file 'filename' in the directory 'dir'.
  // If 'filename' is a fully qualified path, the basename of the file is
  // extracted first to check for its existence in 'dir'.
  // If 'dir' is not a directory, GetFilenamePath() is called on 'dir' to
  // get the directory (thus, you can pass a filename actually and save
  // yourself that step).
  // 'try_fname' is where the fully qualified name/path of the file will be
  // stored if it is found in 'dir'.
  // If try_filename_dirs is true, it will try to find the file using
  // part of its dirs, i.e. if we are looking for c:/foo/bar/bill.txt, it
  // will first look for bill.txt in dir, then in dir/bar, then in dir/foo/bar
  // etc.
  // Return a pointer to the file found (i.e. 'try_fname') or 0 if not.
  static const char* LocateFileInDir(const char *filename, 
                                     const char *dir, 
                                     char *try_fname,
                                     int try_filename_dirs = 0);

  // Description:
  // Check if the file has a given signature (set of bytes).
  static int FileHasSignature(const char *filename, 
                              const char *signature, unsigned long offset = 0);
  
  // Description:
  // Remove file.
  static int RemoveFile(const char* filename);
  
protected:
  vtkKWDirectoryUtilities();
  ~vtkKWDirectoryUtilities();
  
  char** SystemPath;
  char* CWD;
  char* UnixSlashes;
  char* ProgramFound;
  char* SelfPath;
  char* CollapsedDirectory;
  
private:
  vtkKWDirectoryUtilities(const vtkKWDirectoryUtilities&); // Not implemented
  void operator=(const vtkKWDirectoryUtilities&); // Not implemented
};

#endif



