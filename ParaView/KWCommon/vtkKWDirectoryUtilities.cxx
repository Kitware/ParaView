/*=========================================================================

  Module:    vtkKWDirectoryUtilities.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWDirectoryUtilities.h"

#include "vtkObjectFactory.h"
#include "vtkString.h"

#include <ctype.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <limits.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/wait.h>
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#include <direct.h>
static inline const char* Getcwd(char* buf, unsigned int len)
{
  return _getcwd(buf, len);
}
static inline int Chdir(const char* dir)
{
  #if defined(__BORLANDC__)
  return chdir(dir);
  #else
  return _chdir(dir);
  #endif
}
#else
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
static inline const char* Getcwd(char* buf, unsigned int len)
{
  return getcwd(buf, len);
}
static inline int Chdir(const char* dir)
{
  return chdir(dir);
}
#endif

#if defined(_WIN32) && (defined(_MSC_VER) || defined(__BORLANDC__) || defined(__MINGW32__))
#define _unlink unlink
#endif

vtkCxxRevisionMacro(vtkKWDirectoryUtilities, "1.17");
vtkStandardNewMacro(vtkKWDirectoryUtilities);

//----------------------------------------------------------------------------
vtkKWDirectoryUtilities::vtkKWDirectoryUtilities()
{
  this->SystemPath = 0;
  this->UnixSlashes = 0;
  this->ProgramFound = 0;
  this->CollapsedDirectory = 0;
  this->SelfPath = 0;
  this->CWD = 0;
}

//----------------------------------------------------------------------------
vtkKWDirectoryUtilities::~vtkKWDirectoryUtilities()
{
  if(this->CWD)
    {
    delete [] this->CWD;
    }

  if(this->CollapsedDirectory)
    {
    delete [] this->CollapsedDirectory;
    }
  
  if(this->SelfPath)
    {
    delete [] this->SelfPath;
    }
  
  if(this->ProgramFound)
    {
    delete [] this->ProgramFound;
    }
  
  if(this->UnixSlashes)
    {
    delete [] this->UnixSlashes;
    }
  
  if(this->SystemPath)
    {
    char** p = this->SystemPath;
    for(;*p;++p)
      {
      delete [] *p;
      }
    delete [] this->SystemPath;
    }
}

//----------------------------------------------------------------------------
void vtkKWDirectoryUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
const char* vtkKWDirectoryUtilities::GetEnv(const char* key)
{
  return getenv(key);
}

//----------------------------------------------------------------------------
const char* vtkKWDirectoryUtilities::GetCWD()
{
  if(this->CWD)
    {
    delete [] this->CWD;
    }
  char buf[2048];
  this->CWD = vtkString::Duplicate(Getcwd(buf, 2048));
  return this->CWD;
}

//----------------------------------------------------------------------------
const char* vtkKWDirectoryUtilities::ConvertToUnixSlashes(const char* path)
{
  if(this->UnixSlashes)
    {
    delete [] this->UnixSlashes;
    this->UnixSlashes = 0;
    }
  if(!path)
    {
    return 0;
    }
  this->UnixSlashes = vtkString::Duplicate(path);
  vtkIdType length = vtkString::Length(this->UnixSlashes);
  if(length < 1)
    {
    return this->UnixSlashes;
    }
  
  // Convert slashes.
  vtkIdType i;
  for(i=0; i < length; ++i)
    {
    if(this->UnixSlashes[i] == '\\')
      {
      this->UnixSlashes[i] = '/';
      }
    }
  
  // Remove trailing slash.
  if(this->UnixSlashes[length-1] == '/')
    {
    this->UnixSlashes[length-1] = '\0';
    }
  
  // Replace a leading "~" with home directory.
  if(this->UnixSlashes[0] == '~')
    {
    const char* home = this->GetEnv("HOME");
    if(home)
      {
      char* newStr = vtkString::Append(home, this->UnixSlashes+1);
      delete [] this->UnixSlashes;
      this->UnixSlashes = newStr;
      }
    }
  
  return this->UnixSlashes;
}

//----------------------------------------------------------------------------
int vtkKWDirectoryUtilities::FileExists(const char* filename)
{
  struct stat fs;
  if(stat(filename, &fs) != 0) 
    {
    return 0;
    }
  else
    {
    return 1;
    }
}

//----------------------------------------------------------------------------
int vtkKWDirectoryUtilities::FileIsDirectory(const char* name)
{  
  struct stat fs;
  if(stat(name, &fs) == 0)
    {
#if _WIN32
    return ((fs.st_mode & _S_IFDIR) != 0);
#else
    return S_ISDIR(fs.st_mode);
#endif
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
const char*const* vtkKWDirectoryUtilities::GetSystemPath()
{
  if(this->SystemPath)
    {
    return this->SystemPath;
    }
  // adds the elements of the env variable path to the arg passed in
#if defined(_WIN32) && !defined(__CYGWIN__)
  const char* pathSep = ";";
#else
  const char* pathSep = ":";
#endif
  const char* pathEnv = this->GetEnv("PATH");
  if(!pathEnv)
    {
    this->SystemPath = new char*[1];
    this->SystemPath[0] = 0;
    return this->SystemPath;
    }
  
  vtkIdType length = vtkString::Length(pathEnv);
  char* path = 0;
  if((length > 0) && (path[length-1] != pathSep[0]))
    {
    path = vtkString::Append(pathEnv, pathSep);
    ++length;
    }
  else
    {
    path = vtkString::Duplicate(pathEnv);
    }
  vtkIdType i;
  int pathSize = 0;
  for(i=0 ;i < length; ++i)
    {
    if(path[i] == pathSep[0])
      {
      ++pathSize;
      }
    }
  
  this->SystemPath = new char*[pathSize+1];
  this->SystemPath[pathSize] = 0;
  
  int curPath = 0;
  vtkIdType lpos=0;
  while(lpos < length)
    {
    vtkIdType rpos = lpos;
    for(;path[rpos] != pathSep[0];++rpos);
    path[rpos] = '\0';
    const char* upath = this->ConvertToUnixSlashes(path+lpos);
    this->SystemPath[curPath++] = vtkString::Duplicate(upath);
    lpos = rpos+1;
    }
  
  return this->SystemPath;
}

//----------------------------------------------------------------------------
const char* vtkKWDirectoryUtilities::FindProgram(const char* name)
{
  // Find the executable with the given name.  Searches the system
  // path.  Returns the full path to the executable if it is found.
  // Otherwise, 0 is returned.

  if(this->ProgramFound)
    {
    delete [] this->ProgramFound;
    this->ProgramFound = 0;
    }
  
  // See if the executable exists as written.
  if(this->FileExists(name) && !this->FileIsDirectory(name))
    {
    this->ProgramFound = vtkString::Duplicate(name);
    return this->ProgramFound;
    }
  
  char* tryPath = 0;
#ifdef _WIN32
  tryPath = vtkString::Append(name, ".exe");
  if(this->FileExists(tryPath) && !this->FileIsDirectory(tryPath))
    {
    this->ProgramFound = tryPath;
    return this->ProgramFound;
    }
  delete [] tryPath;
#endif
  
  // Get the system search path.
  const char*const* path = this->GetSystemPath();

  // Look through the path for the program.
  char* name1 = vtkString::Append("/", name);
#ifdef _WIN32
  char* name2 = vtkString::Append(name1, ".exe");
#endif
  
  const char*const* p;
  for(p = path; *p; ++p)
    {
    tryPath = vtkString::Append(*p, name1);
    if(this->FileExists(tryPath) && !this->FileIsDirectory(tryPath))
      {
      delete [] name1;
#ifdef _WIN32
      delete [] name2;
#endif
      this->ProgramFound = tryPath;
      return this->ProgramFound;
      }    
    delete [] tryPath;
    
#ifdef _WIN32
    tryPath = vtkString::Append(*p, name2);
    if(this->FileExists(tryPath) && !this->FileIsDirectory(tryPath))
      {
      delete [] name1;
      delete [] name2;
      this->ProgramFound = tryPath;
      return this->ProgramFound;
      }    
    delete [] tryPath;
#endif    
    }
  delete [] name1;
#ifdef _WIN32
  delete [] name2;
#endif
  
  // Couldn't find the program.
  return 0;
}

//----------------------------------------------------------------------------
const char* vtkKWDirectoryUtilities::CollapseDirectory(const char* dir)
{
  if(this->CollapsedDirectory)
    {
    delete [] this->CollapsedDirectory;
    this->CollapsedDirectory = 0;
    }
  
#ifdef _WIN32
  // Ultra-hack warning:
  // This changes to the target directory, saves the working directory,
  // and then changes back to the original working directory.
  char* cwd = vtkString::Duplicate(this->GetCWD());
  if(dir && (vtkString::Length(dir) > 0)) { Chdir(dir); }
  this->CollapsedDirectory = vtkString::Duplicate(this->GetCWD());
  Chdir(cwd);
  delete [] cwd;
  return this->CollapsedDirectory;
#else
# ifdef MAXPATHLEN
  char resolved_name[MAXPATHLEN];
# else
#  ifdef PATH_MAX
  char resolved_name[PATH_MAX];
#  else
  char resolved_name[5024];
#  endif
# endif
  if(dir && (vtkString::Length(dir) > 0))
    {
    realpath(dir, resolved_name);
    this->CollapsedDirectory = vtkString::Duplicate(resolved_name);
    }
  else
    {
    this->CollapsedDirectory = vtkString::Duplicate(this->GetCWD());
    }
  return this->CollapsedDirectory;
#endif  
}

//----------------------------------------------------------------------------
const char* vtkKWDirectoryUtilities::FindSelfPath(const char* argv0)
{
  // Find our own executable's location.
  
  if(this->SelfPath)
    {
    delete [] this->SelfPath;
    this->SelfPath = 0;
    }
  
  // Make sure we have a name.
  if(!argv0)
    {
    return 0;
    }
  char* av0 = vtkString::Duplicate(this->ConvertToUnixSlashes(argv0));
  vtkIdType av0len = vtkString::Length(av0);
  if(av0len < 1)
    {
    delete [] av0;
    return 0;
    }
  
  // Find the last forward slash.
  vtkIdType i;
  vtkIdType pos = -1;
  for(i=av0len-1; i >= 0; --i)
    {
    if(av0[i] == '/')
      {
      pos = i;
      break;
      }
    }
  
  // If no slash, the program must be in the system path.
  if(pos < 0)
    {
    char* newAv0 = vtkString::Duplicate(this->FindProgram(av0));
    delete [] av0;
    av0 = newAv0;
    av0len = vtkString::Length(av0);
    
    // Find the trailing slash, if any.
    for(i=av0len-1; i >= 0; --i)
      {
      if(av0[i] == '/')
        {
        pos = i;
        break;
        }
      }
    }
  
  // If there is a slash, use the directory up to the last slash.
  // Otherwise, the program must be in the current working directory.
  char* selfPath=0;
  if(pos >= 0)
    {
    av0[pos] = '\0';
    selfPath = vtkString::Duplicate(this->CollapseDirectory(av0));
    }
  else
    {
    selfPath = vtkString::Duplicate(this->CollapseDirectory("."));
    }
  delete [] av0;
  this->SelfPath = vtkString::Duplicate(this->ConvertToUnixSlashes(selfPath));
  delete [] selfPath;
  return this->SelfPath;
}

//----------------------------------------------------------------------------
const char* vtkKWDirectoryUtilities::GetFilenamePath(const char *filename, 
                                                     char *path)
{
  if ( !filename || strlen(filename) == 0 )
    {
    path[0] = 0;
    return path;
    }
  const char *ptr = filename + strlen(filename) - 1;
  while (ptr > filename && *ptr != '/' && *ptr != '\\')
    {
    ptr--;
    }

  size_t length = ptr - filename;
  if (length)
    {
    strncpy(path, filename, length);
    }
  path[length] = '\0';
  
  return path;
}

//----------------------------------------------------------------------------
const char* vtkKWDirectoryUtilities::GetFilenameName(const char *filename, 
                                                     char *name)
{
  if ( !filename || strlen(filename) == 0 )
    {
    name[0] = 0;
    return name;
    }
  int found = 0;
  const char *ptr = filename + strlen(filename) - 1;
  while (ptr > filename)
    {
    if (*ptr == '/' || *ptr == '\\')
      {
      found = 1;
      break;
      }
    ptr--;
    }

  strcpy(name, ptr + found);
  
  return name;
}

//----------------------------------------------------------------------------
const char* vtkKWDirectoryUtilities::GetFilenameExtension(
  const char *filename, char *ext)
{
  if (!filename)
    {
    ext[0] = 0;
    }
  else
    {
    char *dot = strrchr(filename, '.');
    if (dot)
      {
      strcpy(ext, dot + 1);
      }
    else
      {
      ext[0] = 0;
      }
    }

  return ext;
}

//----------------------------------------------------------------------------
const char* vtkKWDirectoryUtilities::GetFilenameWithoutExtension(
  const char *filename, char *name)
{
  if (!filename)
    {
    name[0] = 0;
    }
  else
    {
    char *dot = strrchr(filename, '.');
    if (dot)
      {
      strncpy(name, filename, dot - filename);
      name[dot - filename] = '\0';
      }
    else
      {
      strcpy(name, filename);
      }
    }

  return name;
}

//----------------------------------------------------------------------------
const char* vtkKWDirectoryUtilities::LocateFileInDir(const char *filename, 
                                                     const char *dir, 
                                                     char *try_fname,
                                                     int try_filename_dirs)
{
  if (!filename || !dir || !try_fname)
    {
    return 0;
    }

  // Get the basename of 'filename'

  char *filename_base = new char [strlen(filename) + 1];
  vtkKWDirectoryUtilities::GetFilenameName(filename, filename_base);

  // Check if 'dir' is really a directory 
  // If win32 and is like C:, accept it as a dir

  char *real_dir = 0;
  if (!vtkKWDirectoryUtilities::FileIsDirectory(dir))
    {
#if _WIN32
    size_t dir_len = strlen(dir);
    if (dir_len < 2 || dir[dir_len - 1] != ':')
      {
#endif
      real_dir = new char [strlen(dir) + 1];
      dir = vtkKWDirectoryUtilities::GetFilenamePath(dir, real_dir);
#if _WIN32
      }
#endif
    }

  // Try to find the file in 'dir'

  const char *res = 0;
  if (filename_base && dir)
    {
    size_t dir_len = strlen(dir);
    int need_slash = 
      (dir_len && dir[dir_len - 1] != '/' && dir[dir_len - 1] != '\\');
    sprintf(try_fname, "%s%s%s", 
            dir, (need_slash ? "/" : ""), filename_base);
    if (vtkKWDirectoryUtilities::FileExists(try_fname))
      {
      res = try_fname;
      }

    // If not found, we can try harder by appending part of the file to
    // found to the directory to look inside
    // Example: if we were looking for /foo/bar/yo.txt in /d1/d2, then
    // try to find yo.txt in /d1/d2/bar, then /d1/d2/foo/bar, etc.

    else if (try_filename_dirs)
      {
      char *temp = new char [strlen(dir) + 1 + strlen(filename) + 1];

      char *filename_dir = vtkString::Duplicate(filename);
      char *filename_dir_base = new char [strlen(filename) + 1];
      char *filename_dir_bases = new char [strlen(filename) + 1];
      filename_dir_bases[0] = '\0';

      do
        {
        vtkKWDirectoryUtilities::GetFilenamePath(
          filename_dir, filename_dir);
        vtkKWDirectoryUtilities::GetFilenameName(
          filename_dir, filename_dir_base);
#if _WIN32
        size_t len = strlen(filename_dir_base);
        if (!*filename_dir_base || filename_dir_base[len - 1] == ':')
#else
        if (!*filename_dir_base)
#endif
          {
          break;
          }

        sprintf(temp, "%s/%s", filename_dir_base, filename_dir_bases);
        strcpy(filename_dir_bases, temp);

        sprintf(temp, "%s%s%s", dir, (need_slash ?"/":""), filename_dir_bases);
        res = vtkKWDirectoryUtilities::LocateFileInDir(
          filename_base, temp, try_fname, 0);

        } while (!res && *filename_dir_base);

      delete [] filename_dir;
      delete [] filename_dir_base;
      delete [] filename_dir_bases;
      delete [] temp;
      }
    }
    
  // Free mem

  if (filename_base)
    {
    delete [] filename_base;
    }

  if (real_dir)
    {
    delete [] real_dir;
    }

  return res;
}

//----------------------------------------------------------------------------
int vtkKWDirectoryUtilities::FileHasSignature(const char *filename,
                                              const char *signature, 
                                              unsigned long offset)
{
  if (!filename || !signature)
    {
    return 0;
    }

  FILE *fp;
  fp = fopen(filename, "rb");
  if (!fp)
    {
    return 0;
    }

  fseek(fp, offset, SEEK_SET);

  int res = 0;
  size_t signature_len = strlen(signature);
  char *buffer = new char [signature_len];

  if (fread(buffer, 1, signature_len, fp) == signature_len)
    {
    res = (!strncmp(buffer, signature, signature_len) ? 1 : 0);
    }

  delete [] buffer;

  fclose(fp);
  return res;
}

//----------------------------------------------------------------------------
int vtkKWDirectoryUtilities::RemoveFile(const char* filename)
{
  if (!vtkKWDirectoryUtilities::FileExists(filename))
    {
    return 0;
    }

  return unlink(filename) ? 0 : 1;
}

//----------------------------------------------------------------------------
time_t vtkKWDirectoryUtilities::ModifiedTime(const char* filename)
{
  struct stat fs;
  if (stat(filename, &fs) != 0) 
    {
    return 0;
    }
  else
    {
    return fs.st_mtime;
    }
}

//----------------------------------------------------------------------------
time_t vtkKWDirectoryUtilities::CreationTime(const char* filename)
{
  struct stat fs;
  if (stat(filename, &fs) != 0) 
    {
    return 0;
    }
  else
    {
    return fs.st_ctime >= 0 ? fs.st_ctime : 0;
    }
}


