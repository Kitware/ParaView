/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWDirectoryUtilities.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

vtkCxxRevisionMacro(vtkKWDirectoryUtilities, "1.1.2.2");
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
